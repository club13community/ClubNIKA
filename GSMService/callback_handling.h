//
// Created by independent-variable on 5/21/2024.
//

#pragma once
#include <stdint.h>

namespace gsm {
	enum class Event : uint32_t {
		CALL_STATE_CHANGED = 1U << 0,
		KEY_PRESSED = 1U << 1
	};

	void init_callback_handling();

	void handle(Event event);
}