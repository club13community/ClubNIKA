//
// Created by independent-variable on 5/21/2024.
//

#pragma once
#include "sim900.h"
#include <stdint.h>

namespace gsm {
	extern volatile sim900::CardStatus card_status;
	extern volatile sim900::Registration registration;
	extern volatile uint8_t signal_strength;

	enum class Task : uint8_t {
		TURN_OFF = 1U << 0,
		TURN_ON = 1U << 1,
		REBOOT = 1U << 2,
		/** Checks network connection, signal strength, SIM card status, etc. */
		CHECK_MODULE_STATE = 1U << 3
	};

	void init_service_tasks();

	void schedule(Task task);
	void execute_or_schedule(gsm::Task task);

	/** Executes scheduled tasks. Use this before releasing a mutex(it releases mutex it self if nothing to execute). */
	void execute_scheduled();
}