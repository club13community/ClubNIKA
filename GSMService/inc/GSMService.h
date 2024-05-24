#pragma once

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"

namespace gsm {
	/** Interrupts should be enabled(uses timer to discharge decoupling cap.)*/
	void init_periph();
	void start();

	/** Result of outgoing call initiation. */
	enum class Dialing {
		/** Recipient picked up a phone. */
		DONE,
		/** Recipient pressed 'reject' or line is busy. */
		REJECTED,
		/** Recipient did not pick up a phone or is not in network. */
		NO_ANSWER,
		/** Failed to make a call: not registered in mobile network, some call is already ongoing,
		 * some problem with GSM module, etc. */
		ERROR
	};

	enum class Direction {INCOMING, OUTGOING};

	/** Collection of controlling methods. */
	class Controls {
	public:
		virtual Dialing call(const char * phone) = 0;
		virtual void end_call() = 0;
		/** @return ID of SMS or -1 if failed to send. */
		virtual int16_t send_sms(const char * phone) = 0;
	};

	/** Blocks thread till control is released by other.
	 * @returns API which always runs requested action(and always returns true) */
	//Controls & get_ctrl();

	/** @returns value in range [0, 100]. If not registered in mobile network - signal strength is 0 too. */
	uint8_t get_signal_strength();

	void set_on_incoming_call(void (* callback)(char * phone));
	/** Order of callbacks for both incoming and outgoing: set_on_call_dialed(...) -> set_on_call_ended(...). */
	void set_on_call_dialed(void (* callback)(Direction direction));
	/** Order of callbacks for both incoming and outgoing: set_on_call_dialed(...) -> set_on_call_ended(...).
	 * If call is ended by method end_call() - this callback is not invoked. */
	void set_on_call_ended(void (* callback)());
}
