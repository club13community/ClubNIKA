//
// Created by independent-variable on 5/21/2024.
//

#pragma once
#include <stdint.h>

namespace gsm {
	enum class Event : uint32_t {
		CALL_STATE_CHANGED = 1U << 1,
		ERROR = 1U << 4,
		/** Card was functional, but non-functional now(not issued if card was not functional before) */
		CARD_ERROR = 1U << 5,
		/** There was established/establishing connection to mobile network, but now it is absent. */
		NETWORK_ERROR = 1U << 6
	};

	void init_event_handling();
	void reboot_event_handling();

	void handle(Event event);
}