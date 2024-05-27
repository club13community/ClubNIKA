//
// Created by independent-variable on 5/26/2024.
// Helper stuff for commands execution.
//

#pragma once
#include "./state.h"
#include <stdint.h>

namespace gsm {
	enum class AsyncTask : uint8_t {
		TURN_OFF = 1U << 0,
		TURN_ON = 1U << 1,
		REBOOT = 1U << 2,
		/** Checks network connection, signal strength, SIM card status, etc. */
		CHECK_MODULE_STATE = 1U << 3,
		DELETE_INCOMING_SMS = 1U << 4
	};

	void schedule_reboot();
	/** Will try to capture control and reboot. If fails to capture - schedule reboot. */
	void reboot_asap();
	void check_module_state_asap();
	void delete_incoming_sms_asap();

	/** Use this before releasing a mutex(it releases mutex it self if nothing scheduled). */
	void executed();

	void executed(AsyncTask task);

	void clear_pending(AsyncTask task);
}