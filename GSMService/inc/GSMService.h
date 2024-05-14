#pragma once

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"

namespace gsm {
	void init_periph();
	void start();

	/** Result of outgoing call initiation. */
	enum class Dialing {
		/** Recipient picked up a phone. */
		DONE,
		/** Recipient pressed 'reject'. */
		REJECTED,
		/** Recipient did not pick up or some problem with SIM900. */
		FAILED
	};

	/** Stage of registration in mobile network. */
	enum class Registration {
		/** Registered(in domestic or roaming). */
		DONE,
		ONGOING,
		FAILED
	};

	enum class Result {DONE, FAILED};

	enum class Error {ERROR_RESPONSE, CORRUPTED_RESPONSE, NO_RESPONSE, NONE};

	struct Controls;
	typedef void (* Handler)(const Controls & ctrl);
	typedef void (* ResultHandler)(Result res, Controls & ctrl);
	typedef void (* DialingHandler)(Dialing res, Controls & ctrl);
	typedef void (* CallHandler)(const char * phone, Controls & ctrl);
	typedef void (* SignalHandler)(uint8_t level_pct, Controls & ctrl);
	typedef void (* RegistrationHandler)(Registration reg, Controls & ctrl);

	/** Collection of methods. Each method returns true if driver is not busy and action is taken to execution. */
	struct Controls {
		/** @param phone '\0' ended phone number without leading '+' and country code. */
		bool (* const call)(const char * phone, DialingHandler handler);
		/** If no incoming call - do not invoke handler(if there was incoming call,
		 * but ended before accepting - appropriate callback is invoked) */
		bool (* const accept_call)(Handler handler);
		/** If no incoming call - do not invoke handler(if there was incoming call,
		 * but ended before rejecting - appropriate callback is invoked) */
		bool (* const reject_call)(Handler handler);
		/** If no ongoing call - do not invoke handler(if there was ongoing call,
		 * but ended before ending - appropriate callback is invoked) */
		bool (* const end_call)(Handler handler);
		/** @param message '\0' message, latin letters only.
	 	 * @param phone '\0' ended phone number without leading '+' and country code. */
		bool (* const send_sms)(const char * message, const char * phone, ResultHandler handler); // todo define max length
		/** Deletes all SMS(sent, unread, read, etc.) */
		bool (* const delete_all_sms)(ResultHandler handler);
		bool (* const get_signal_strength)(SignalHandler handler);
		bool (* const get_registration)(RegistrationHandler handler);
	};

	/** Interrupts should be enabled(uses timer to discharge decoupling cap.)*/
	void init_periph();
	void start();

	/** Blocks thread till control is released by other.
	 * @returns API which always runs requested action(and always returns true) */
	Controls & get_ctrl();
	/** Does not block execution.
	 * If control is already taken - returned API never executes requested(and always returns false) */
	Controls & try_get_ctrl();


}

