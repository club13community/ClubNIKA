//
// Created by independent-variable on 5/11/2024.
//

#pragma once
#include <stdint.h>

namespace sim900 {
	/** Interrupts should be enabled(uses timer to discharge decoupling cap.)*/
	void init_periph();
	void start();

	enum class Result {
		OK = 0,
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
	enum class CallDirection {
		INCOMING,
		OUTGOING
	};
	enum class CallState {
		RINGING,
		/** Interlocutor picked up a phone. */
		SPEAKING,
		/** No ongoing calls(all previous are ended). */
		ENDED,
		/** One more incoming call. */
		WAITING,
		/** Put on hold. */
		HELD
	};
	enum class CallEnd {
		/** Interlocutor hanged up, or not in network while outgoing call */
		NORMAL,
		/** Rejected by interlocutor or line is busy. */
		BUSY,
		/** Interlocutor do not answer for too long. */
		NO_ANSWER,
		/** GSM module is not ready for calls. */
		NETWORK_ERROR
	};

	void turn_on(void (* callback)(bool success));
	void turn_off(void (* callback)());
	bool is_turned_on();
	bool is_turned_off();
	void get_card_status(void (* callback)(CardStatus status, Result result));
	void get_signal_strength(void (* callback)(uint8_t signal_pct, Result result));
	void get_registration(void (* callback)(Registration registration, Result result));
	void send_sms(const char * phone, const char * text, void (* callback)(uint16_t id, Result result));
	void delete_sent_sms(void (* callback)(Result result));
	void delete_received_sms(void (* callback)(Result result));
	void delete_all_sms(void (* callback)(Result result));
	void call(const char * phone, void (* callback)(Result result));
	/** If no incoming call - do not invoke handler(if there was incoming call,
	 * but ended before accepting - appropriate callback is invoked) */
	void accept_call(void (* callback)(Result result));
	/** If no ongoing call - do not invoke handler(if there was ongoing call,
	 * but ended before ending - appropriate callback is invoked) */
	void end_call(void (* callback)(Result result));
	/** Enables keyboard input detection during a call.
	 * @param debounce_ms min. time between pressing the same button, should be in range [0, 10'000] */
	void enable_dtmf(uint16_t debounce_ms, void (* callback)(Result result));
	void disable_dtmf(void (* callback)(Result result));
	/** Waits while call reaches desired state.
	 * @param predicate defines desired state. */
	void wait_call_state(
			bool (* predicate)(uint8_t index, CallState state, CallDirection direction, const char * number),
			uint32_t deadline_ms, void (* callback)(bool state_reached));
	void wait_call_end(uint32_t deadline_ms, void (* callback)(bool ended, CallEnd end_type));
	/** Number in callback's arg. - in local format. Does not end with "ERROR" even if SIM-card is not functioning
	 * or no network connection.
	 * @param data_callback - invoked for every existing call.
	 * @param result_callback - invoked at the end. */
	void get_call_info(void (* data_callback)(uint8_t index, CallState state, CallDirection direction, char * number),
			void (* result_callback)(Result result));
}