//
// Created by independent-variable on 5/21/2024.
//
#include "./callback_handling.h"
#include "periph_allocation.h"
#include "FreeRTOS.h"
#include "task.h"
#include "./service.h"
#include "./state.h"

namespace gsm {
	volatile CallPhase handled_call_phase = CallPhase::ENDED;
}

static volatile TaskHandle_t task;
static void handle_events(void * arg);

void gsm::init_callback_handling() {
	constexpr size_t stack_size = 256;
	static StaticTask_t task_ctrl;
	static StackType_t task_stack[stack_size];
	task = xTaskCreateStatic(handle_events, "gsm", stack_size, nullptr, TASK_NORMAL_PRIORITY, task_stack, &task_ctrl);
}

void gsm::handle(Event event) {
	xTaskNotify(task, (uint32_t)event, eSetBits);
}

using namespace gsm;

static void handle_events();
static void handle_events(void * arg) {
	gsm::turn_on();
	while (true) {
		handle_events();
	}
}

static inline uint32_t flag_of(Event event) {
	return (uint32_t)event;
}

static void handle_events() {
	using namespace gsm;
	constexpr uint32_t ALL_BITS = 0xFF'FF'FF'FFU;
	uint32_t bits;
	while (xTaskNotifyWait(0, ALL_BITS, &bits, portMAX_DELAY) == pdFALSE);

	if (bits & flag_of(Event::KEY_PRESSED)) {
		if (handled_call_phase == CallPhase::SPEAKING) {
			safe_on_key_pressed(pressed_key);
		}
	}

	if (bits & flag_of(Event::CALL_STATE_CHANGED)) {
		CallPhase handled_now = handled_call_phase;
		portENTER_CRITICAL();
		CallPhase actual_now = actual_call_phase;
		Direction direction_now = call_direction;
		portEXIT_CRITICAL();
		if (handled_now == CallPhase::ENDED && actual_now == CallPhase::RINGING) {
			handled_call_phase = CallPhase::RINGING;
			if (direction_now == Direction::INCOMING) {
				safe_on_incoming_call(phone_number);
			}
		} else if (one_of(handled_now, CallPhase::ENDED, CallPhase::RINGING) && actual_now == CallPhase::SPEAKING) {
			handled_call_phase = CallPhase::SPEAKING;
			safe_on_call_dialed(call_direction);
		} else if (handled_now == CallPhase::RINGING && actual_call_phase == CallPhase::ENDED) {
			// do not invoke on_call_ended() because on_call_dialed() was not invoked
			handled_call_phase = CallPhase::ENDED;
		} else if (handled_now == CallPhase::SPEAKING && actual_now == CallPhase::ENDED) {
			handled_call_phase = CallPhase::ENDED;
			safe_on_call_ended();
		} // else handled_now == actual_now
	}
}

