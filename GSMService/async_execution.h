//
// Created by independent-variable on 5/26/2024.
//

#pragma once
#include "./state.h"
#include <stdint.h>

namespace gsm {
	enum class Task : uint8_t {
		TURN_OFF = 1U << 0,
		TURN_ON = 1U << 1,
		REBOOT = 1U << 2,
		/** Checks network connection, signal strength, SIM card status, etc. */
		CHECK_MODULE_STATE = 1U << 3
	};

	void schedule_reboot();
	void reboot_asap();
	void check_module_state_asap();

	/** Executes scheduled tasks. Use this before releasing a mutex(it releases mutex it self if nothing to execute). */
	void execute_scheduled();

	void executed(Task task);

	void clear_pending(Task task);
}