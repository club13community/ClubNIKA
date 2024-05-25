#pragma once

#include <stdint.h>

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
		friend Controls & get_ctrl();
	private:
		static Controls inst;
		Controls(){}
	public:
		Dialing call(const char * phone);
		void end_call();
		bool accept_call();
		/** @return ID of SMS or -1 if failed to send. */
		int16_t send_sms(const char * phone);
	};

	/** Blocks thread till control is released by other.
	 * @returns set of APIs, always invoke something - otherwise mutex stays taken. */
	Controls & get_ctrl();

	/** @returns value in range [0, 100]. If not registered in mobile network - signal strength is 0 too. */
	uint8_t get_signal_strength();

	void set_on_incoming_call(void (* callback)(char * phone));
	/** Order of callbacks for both incoming and outgoing: set_on_call_dialed(...) -> set_on_call_ended(...). */
	void set_on_call_dialed(void (* callback)(Direction direction));
	/** Order of callbacks for both incoming and outgoing: set_on_call_dialed(...) -> set_on_call_ended(...).
	 * If call is ended by method end_call() - this callback is not invoked. */
	void set_on_call_ended(void (* callback)());
}
