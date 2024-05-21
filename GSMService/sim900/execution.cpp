//
// Created by independent-variable on 5/11/2024.
//
#include "sim900.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "periph_allocation.h"
#include "./execution.h"
#include "./power_ctrl.h"
#include "./uart_ctrl.h"
#include "./uart_callbacks.h"
#include "./listeners.h"
#include "logging.h"

#if configTIMER_TASK_PRIORITY <= SIM900_DRIVER_PRIORITY
#error Expected that timer API takes effect after return.
#endif

namespace sim900 {
	tx_buffer_t tx_buffer;
}

void sim900::init_periph() {
	init_power_ctrl();
	init_uart_ctrl();
}

using namespace sim900;

#define MESSAGE_RECEIVED	( (uint32_t)1U )
#define TIMEOUT				( (uint32_t)2U )
#define MESSAGE_SENT		( (uint32_t)4U )
#define ALL_EVENTS			(MESSAGE_RECEIVED | TIMEOUT | MESSAGE_SENT)

static TaskHandle_t service_task;

static TimerHandle_t timer;
static volatile enum {NONE, RESPONSE, DELAY,
	/** value used while checking if handler manipulated with timeout */
	RESERVED} timeout_type;
static void (* volatile timeout_elapsed)();

static volatile ResponseListener message_received;

static void (* volatile message_sent)();

static void service_commands(void * args);
static void notify_timeout(TimerHandle_t xTimer);
static BaseType_t notify_sent();

void sim900::start() {
	message_received = nullptr;

	static StaticTimer_t timer_ctrl;
	timer = xTimerCreateStatic("sim900 tim", 1, pdFALSE, (void *) 0, notify_timeout, &timer_ctrl);
	timeout_type = NONE;

	constexpr size_t stack_size = 512;
	static StackType_t stack[stack_size];
	static StaticTask_t task_ctrl;
	service_task = xTaskCreateStatic(service_commands, "sim900", stack_size, nullptr,
									 SIM900_DRIVER_PRIORITY, stack, &task_ctrl);
}

void sim900::begin_command(ResponseListener listener) {
	message_received = listener;
}

void sim900::end_command() {
	message_received = nullptr;
}

void sim900::send_with_timeout(const char * cmd, uint16_t len, uint32_t deadline_ms, void (* timeout_elapsed)()) {
	portENTER_CRITICAL();
	start_response_timeout(deadline_ms, timeout_elapsed);
	send(cmd, len);
	portEXIT_CRITICAL();
}

void sim900::send_with_timeout(const char * cmd, uint16_t len, void (* message_sent)(),
							   uint32_t deadline_ms, void (* timeout_elapsed)()) {
	::message_sent = message_sent;
	portENTER_CRITICAL();
	start_response_timeout(deadline_ms, timeout_elapsed);
	send(cmd, len, notify_sent);
	portEXIT_CRITICAL();
}

void sim900::start_response_timeout(uint32_t deadline_ms, void (* timeout_elapsed)()) {
	timeout_type = RESPONSE;
	::timeout_elapsed = timeout_elapsed;
	xTimerChangePeriod(timer, pdMS_TO_TICKS(deadline_ms), portMAX_DELAY);
}

void sim900::start_timeout(uint32_t delay_ms, void (* timeout_elapsed)()) {
	timeout_type = DELAY;
	::timeout_elapsed = timeout_elapsed;
	xTimerChangePeriod(timer, pdMS_TO_TICKS(delay_ms), portMAX_DELAY);
}

void sim900::stop_timeout() {
	timeout_type = NONE;
	xTimerStop(timer, portMAX_DELAY);
	ulTaskNotifyValueClear(service_task, TIMEOUT);
}

BaseType_t sim900::on_received() {
	BaseType_t task_woken;
	xTaskNotifyFromISR(service_task, MESSAGE_RECEIVED, eSetBits, &task_woken);
	return task_woken;
}

static void notify_timeout(TimerHandle_t xTimer) {
	xTaskNotify(service_task, TIMEOUT, eSetBits);
}

static BaseType_t notify_sent() {
	BaseType_t task_woken = pdFALSE;
	xTaskNotifyFromISR(service_task, MESSAGE_SENT, eSetBits, &task_woken);
	return task_woken;
}

static inline void service_timeout() {
	timeout_type = NONE;
	timeout_elapsed();
}

static inline void service_response() {
	do {
		bool handled = false;
		if (message_received != nullptr) {
			if (timeout_type != RESPONSE) {
				handled = message_received(rx_buffer);
			} else {
				timeout_type = RESERVED;
				handled = message_received(rx_buffer);
				if (timeout_type != RESERVED) {
					// handler manipulated with timeouts - do not override it
				} else if (!handled) {
					// not handled - continue timeout
					timeout_type = RESPONSE;
				} else {
					// handled - stop timeout
					xTimerStop(timer, portMAX_DELAY);
					ulTaskNotifyValueClear(service_task, TIMEOUT);
				}
			}
		}
		handled = handled || ring_listener(rx_buffer) || timestamp_listener(rx_buffer) || ignoring_listener(rx_buffer);
		if (!handled) {
			if (rx_buffer.is_message_ok()) {
				char message_part[11];
				rx_buffer.copy(message_part, 10);
				rec::log("Unexpected from SIM900: {0}", {message_part});
			} else {
				rec::log("Not executed corrupted msg. from SIM900");
			}
		}
	} while (rx_buffer.next_read());
}

static void service_commands(void * args) {
	while (true) {
		uint32_t bits;
		while (xTaskNotifyWait(0, ALL_EVENTS, &bits, portMAX_DELAY) == pdFALSE);
		if (bits & MESSAGE_SENT) {
			message_sent();
		}
		if (bits & TIMEOUT) {
			service_timeout();
		}
		if (bits & MESSAGE_RECEIVED && rx_buffer.may_read()) {
			service_response();
		}
	}
}