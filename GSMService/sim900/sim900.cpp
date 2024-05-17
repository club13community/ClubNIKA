//
// Created by independent-variable on 5/11/2024.
//
#include "sim900.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "timers.h"
#include "periph_allocation.h"
#include "./execution.h"
#include "./power_ctrl.h"
#include "./uart_ctrl.h"

#if configTIMER_TASK_PRIORITY <= SIM900_DRIVER_PRIORITY
#error Expected that timer API takes effect after return.
#endif

namespace sim900 {
	tx_buffer_t tx_buffer;
	rx_buffer_t rx_buffer;
}

void sim900::init_periph() {
	init_power_ctrl();
	init_uart_ctrl();
}

using namespace sim900;

#define SIM900_MESSAGE	( (uint32_t)1U )
#define TIMEOUT			( (uint32_t)2U )
#define METHOD			( (uint32_t)4U )
#define ALL_EVENTS		(SIM900_MESSAGE | TIMEOUT | METHOD)

static TaskHandle_t service_task;

static TimerHandle_t timer;
static volatile enum {NONE, RESPONSE, DELAY} timeout_type;
static void (* volatile timeout_handler)();

static volatile ResponseHandler response_handler;

static void (* volatile ctrl_method)();

static void service_commands(void * args);
static void timeout_elapsed(TimerHandle_t xTimer);

void sim900::start() {
	response_handler = nullptr;

	static StaticTimer_t timer_ctrl;
	timer = xTimerCreateStatic("sim900 tim", 0, pdFALSE, (void *)0, timeout_elapsed, &timer_ctrl);
	timeout_type = NONE;

	constexpr size_t stack_size = 256;
	static StackType_t stack[stack_size];
	static StaticTask_t task_ctrl;
	service_task = xTaskCreateStatic(service_commands, "sim900", stack_size, nullptr,
									 SIM900_DRIVER_PRIORITY, stack, &task_ctrl);
}

void sim900::response_received() {
	xTaskNotify(service_task, SIM900_MESSAGE, eSetBits);
}

void sim900::begin_command(ResponseHandler handler) {
	response_handler = handler;
}

void sim900::change_listener(ResponseHandler handler) {
	response_handler = handler;
}

void sim900::end_command() {
	response_handler = nullptr;
}

void sim900::send_with_timeout(const char * cmd, uint16_t len, uint32_t deadline_ms, void (* timeout_elapsed)()) {
	// todo think about critical section
	start_response_timeout(deadline_ms, timeout_elapsed);
	send(cmd, len);
}

void sim900::start_response_timeout(uint32_t deadline_ms, void (* timeout_elapsed)()) {
	timeout_type = RESPONSE;
	timeout_handler = timeout_elapsed;
	xTimerChangePeriod(timer, pdMS_TO_TICKS(deadline_ms), portMAX_DELAY);
}

void sim900::start_timeout(uint32_t delay_ms, void (* callback)()) {
	timeout_type = DELAY;
	timeout_handler = callback;
	xTimerChangePeriod(timer, pdMS_TO_TICKS(delay_ms), portMAX_DELAY);
}

void sim900::stop_timeout() {
	timeout_type = NONE;
	xTimerStop(timer, portMAX_DELAY);
	ulTaskNotifyValueClear(service_task, TIMEOUT);
}

BaseType_t sim900::invoke_from_task(void (* method)()) {
	ctrl_method = method;
	BaseType_t task_woken = pdFALSE;
	xTaskNotifyFromISR(service_task, METHOD, eSetBits, &task_woken);
	return task_woken;
}

static inline void service_timeout() {
	timeout_type = NONE;
	timeout_handler();
}

static inline void service_response() {
	do {
		bool handled = false;
		if (response_handler != nullptr) {
			if (timeout_type != RESPONSE) {
				handled = response_handler(rx_buffer);
			} else {
				timeout_type = NONE;
				handled = response_handler(rx_buffer);
				if (!handled) {
					// not handled - continue timeout
					timeout_type = RESPONSE;
				} else if (timeout_type == NONE) {
					// handled and did not start another
					xTimerStop(timer, portMAX_DELAY);
					ulTaskNotifyValueClear(service_task, TIMEOUT);
				} // else - handled and configured timer for another
			}
		}
	} while (rx_buffer.next_read());
}

static void service_commands(void * args) {
	while (true) {
		uint32_t bits;
		while (xTaskNotifyWait(0, ALL_EVENTS, &bits, portMAX_DELAY) == pdFALSE);
		if (bits & METHOD) {
			ctrl_method();
		}
		if (bits & TIMEOUT) {
			service_timeout();
		}
		if (bits & SIM900_MESSAGE && rx_buffer.may_read()) {
			service_response();
		}
	}
}

static void timeout_elapsed(TimerHandle_t xTimer) {
	xTaskNotify(service_task, TIMEOUT, eSetBits);
}