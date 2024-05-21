//
// Created by independent-variable on 5/21/2024.
//

#pragma once
#include <stdint.h>

namespace gsm {
	enum class Task : uint8_t {
		REBOOT = 1U,
		/** Checks network connection, signal strength, SIM card status, etc. */
		CHECK_MODULE_STATE = 2U,
		GET_INCOMING_PHONE = 4U, CHECK_CALL_STATE = 8U};

	void init_task_execution();

	/** Turns GSM module at the very beginning. 'Gives' mutex at the end(do not 'give' mutex after it's creation). */
	void turn_module_on();

	/** Executes task now or after mutex is released */
	void execute_or_schedule(Task task);

	/** Executes scheduled tasks. Use this before releasing a mutex(it releases mutex it self if nothing to execute). */
	void execute_scheduled();

	/** Starts timer for polling call status. */
	void poll_call_status();

	/** Starts timer to poll network connection, signal quality, card status, etc. */
	void poll_module_status();
	void stop_module_status_polling();
}