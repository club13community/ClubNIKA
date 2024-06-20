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
	volatile CallPhase handled_call_phase = CallPhase::NONE;
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

	CallPhase handled_now = handled_call_phase;
	portENTER_CRITICAL();
	CallPhase actual_now = actual_call_phase;
	Direction direction_now = call_direction;
	portEXIT_CRITICAL();

	if (bits & flag_of(Event::KEY_PRESSED)) {
		if (handled_now == CallPhase::SPEAKING) {
			safe_on_key_pressed(pressed_key);
		}
	}

	if (bits & flag_of(Event::CALL_STATE_CHANGED)) {
		if (actual_now == CallPhase::RINGING && handled_now != CallPhase::RINGING) {
			handled_call_phase = CallPhase::RINGING;
			if (direction_now == Direction::INCOMING) {
				safe_on_incoming_call(phone_number);
			}
		} else if (actual_now == CallPhase::SPEAKING && handled_now != CallPhase::SPEAKING) {
			handled_call_phase = CallPhase::SPEAKING;
			safe_on_call_dialed(call_direction);
		} else if (actual_call_phase == CallPhase::ENDED) {
			handled_call_phase = CallPhase::NONE; // jump over "ENDED"(as it is used only for "actual phase")
			actual_call_phase = CallPhase::NONE; // nobody writes to this var. after phase "ENDED"
			if (handled_now == CallPhase::SPEAKING) {
				safe_on_call_ended();
			} // else - don't invoke "on call ended" because "on call dialed" was not invoked
		} else {
			handled_call_phase = actual_now;
		}
	}
}

