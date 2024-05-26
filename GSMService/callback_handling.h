//
// Created by independent-variable on 5/21/2024.
//

#pragma once
#include <stdint.h>
#include "./call.h"

namespace gsm {
	enum class Event : uint32_t {
		CALL_STATE_CHANGED = 1U << 0,
		KEY_PRESSED = 1U << 1
	};

	extern volatile CallPhase handled_call_state;

	void init_callback_handling();

	void handle(Event event);
}