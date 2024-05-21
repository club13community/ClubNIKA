//
// Created by independent-variable on 5/21/2024.
//
#include "./event_handling.h"
#include "periph_allocation.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "./tasks.h"
#include "./state.h"
#include "./config.h"

static volatile TaskHandle_t task;
static void handle_events(void * arg);

static volatile TimerHandle_t conn_recovery_timer;
static void conn_recovery_timeout(TimerHandle_t timer);

void gsm::init_event_handling() {
	constexpr size_t stack_size = 256;
	static StaticTask_t task_ctrl;
	static StackType_t task_stack[stack_size];
	task = xTaskCreateStatic(handle_events, "gsm", stack_size, nullptr, TASK_NORMAL_PRIORITY, task_stack, &task_ctrl);

	static StaticTimer_t timer_ctrl;
	conn_recovery_timer = xTimerCreateStatic("gsm conn", pdMS_TO_TICKS(CONNECTION_RECOVERY_TIME_ms),
											 pdFALSE, (void *)0, conn_recovery_timeout, &timer_ctrl);
}

void gsm::handle(Event event) {
	xTaskNotify(task, (uint32_t)event, eSetBits);
}

using namespace gsm;

static void handle_events();
static void handle_events(void * arg) {
	gsm::turn_module_on();
	while (true) {
		handle_events();
	}
}

static inline uint32_t to_int(Event event) {
	return (uint32_t)event;
}

static void handle_events() {
	constexpr uint32_t ALL_BITS = 0xFF'FF'FF'FFU;
	uint32_t bits;
	while (xTaskNotifyWait(0, ALL_BITS, &bits, portMAX_DELAY) == pdFALSE);
	if (bits & to_int(Event::TURNED_ON)) {
		// todo set is_functional flag
		poll_module_status();
	}
	if (bits & to_int(Event::ERROR)) {
		// todo need to reboot
		__NOP();
	}
	if (bits & (to_int(Event::CARD_ERROR) | to_int(Event::NETWORK_ERROR))) {
		xTimerReset(conn_recovery_timer, portMAX_DELAY); // this starts timer
	}
}

static void conn_recovery_timeout(TimerHandle_t timer) {
	using namespace sim900;
	if (card_status != CardStatus::READY || registration == sim900::Registration::FAILED) {
		execute_or_schedule(Task::REBOOT);
	}
}