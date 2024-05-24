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

static volatile TimerHandle_t connection_recovery_timer;
static void connection_recovery_timeout(TimerHandle_t timer);

void gsm::init_event_handling() {
	constexpr size_t stack_size = 256;
	static StaticTask_t task_ctrl;
	static StackType_t task_stack[stack_size];
	task = xTaskCreateStatic(handle_events, "gsm", stack_size, nullptr, TASK_NORMAL_PRIORITY, task_stack, &task_ctrl);

	static StaticTimer_t timer_ctrl;
	connection_recovery_timer = xTimerCreateStatic("gsm conn", pdMS_TO_TICKS(CONNECTION_RECOVERY_TIME_ms),
												   pdFALSE, (void *) 0, connection_recovery_timeout, &timer_ctrl);
}

void gsm::reboot_event_handling() {
	xTimerStop(connection_recovery_timer, portMAX_DELAY);
	xTaskNotifyStateClear(task);
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

	if (bits & to_int(Event::ERROR)) {
		// todo need to reboot, end call if ongoing
		__NOP();
	}

	if (bits & (to_int(Event::CARD_ERROR) | to_int(Event::NETWORK_ERROR))) {
		xTimerReset(connection_recovery_timer, portMAX_DELAY); // this starts timer
	}

	if (bits & to_int(Event::CALL_STATE_CHANGED)) {
		using sim900::CallState, sim900::CallDirection;
		take_call_mutex();
		CallState handled_now = handled_call_state;
		CallState actual_now = actual_call_state;
		CallDirection direction_now = call_direction;
		if (handled_now == CallState::ENDED && actual_now == CallState::RINGING
				&& direction_now == CallDirection::INCOMING) { // for OUTGOING initiated by "start call"
			handled_call_state = CallState::RINGING;
			give_call_mutex();
			safe_on_incoming_call(phone_number);
		} else if (handled_now == CallState::RINGING && actual_now == CallState::ENDED
				&& direction_now == CallDirection::INCOMING) { // for OUTGOING handled by "start call"
			handled_call_state = CallState::ENDED;
			give_call_mutex();
			safe_on_call_ended();
		} else if (handled_now == CallState::SPEAKING && actual_now == CallState::ENDED) {
			handled_call_state = CallState::ENDED;
			give_call_mutex();
			safe_on_call_ended();
		} else {
			give_call_mutex();
		}
		// transition RINGING - SPEAKING is always initiated by API user
	}
}

static void connection_recovery_timeout(TimerHandle_t timer) {
	using namespace sim900;
	if (card_status != CardStatus::READY || registration == sim900::Registration::FAILED) {
		execute_or_schedule(Task::REBOOT);
	}
}