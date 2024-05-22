//
// Created by independent-variable on 5/11/2024.
//

#pragma once
#include <stdint.h>
// todo reset rx buffer on power_off
namespace sim900 {
	/** Interrupts should be enabled(uses timer to discharge decoupling cap.)*/
	void init_periph();
	void start();

	enum class Result {
		OK,
		/** Received "ERROR" from SIM900 */
		ERROR,
		/** Problem with UART communication.
		 * In case of request - data is invalid. In case of operation initiation - not clear if initiated */
		CORRUPTED_RESPONSE,
		/** Seems, that SIM900 stuck */
		NO_RESPONSE
	};
	enum class CardStatus {
		ABSENT,
		/** Not usable till PIN is entered */
		LOCKED,
		READY,
		/** Not functioning */
		ERROR
	};
	enum class Registration {
		/** Registered(in domestic or roaming). */
		DONE,
		ONGOING,
		FAILED
	};
	enum class CallState {
		RINGING,
		/** Interlocutor picked up a phone. */
		DIALED, // todo change to SPEAKING
		/** No ongoing calls(all previous are ended) */
		ENDED
	};
	enum class CallEnd {
		/** Somebody hanged up. */
		NORMAL,
		/** Interlocutor has ongoing call. */
		BUSY,
		/** Interlocutor do not answer for too long. */
		NO_ANSWER
	};

	void turn_on(void (* callback)(bool success));
	void turn_off(void (* callback)());
	bool is_turned_on();
	bool is_turned_off();
	void get_card_status(void (* callback)(CardStatus status, Result result));
	void get_signal_strength(void (* callback)(uint8_t signal_pct, Result result));
	void get_registration(void (* callback)(Registration registration, Result result));
	void send_sms(const char * phone, const char * text, void (* callback)(uint16_t id, Result result));
	void call(const char * phone, void (* callback)(Result result));
	/** If no incoming call - do not invoke handler(if there was incoming call,
	 * but ended before accepting - appropriate callback is invoked) */
	void accept_call(void (* callback)(Result result));
	/** If no ongoing call - do not invoke handler(if there was ongoing call,
	 * but ended before ending - appropriate callback is invoked) */
	void end_call(void (* callback)(Result result));
	/** Number in callback's arg. may contain leading '+' with country code.
	 * Does not end with "ERROR" even if SIM-card is not functioning or no network connection.*/
	void get_call_info(void (* callback)(CallState state, char * number, Result result));
}