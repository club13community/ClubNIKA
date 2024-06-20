//
// Created by independent-variable on 5/24/2024.
//

#pragma once
#include "GSMService.h"
#include "sim900.h"
#include "settings.h"

namespace gsm {
	enum class CallPhase {
		/** No ongoing calls. */
		NONE,
		/** Received phone number of incoming call. */
		// added because if to accept a call right after "call info"(+CLCC:...) message
		// - SIM900 may respond with "ERROR"; now "on incoming call" callback is invoked after "RING" message
		STARTED,
		RINGING,
		/** Interlocutor picked up a phone. */
		SPEAKING,
		ENDED
	};

	extern volatile CallPhase actual_call_phase;
	extern volatile Direction call_direction;
	extern char phone_number[MAX_PHONE_LENGTH + 1];
	/** Key pressed during phone call. */
	extern char pressed_key;

	void end_call();
	/** @returns true if incoming call is accepted(now you may speak). */
	bool accept_call();
	Dialing call(const char *number);
	/** Use during after GSM module turned off(there should be no call-related callbacks).
	 * Will emulate ending of any ongoing call. */
	void terminate_calls();

	inline bool is_ongoing(CallPhase phase) {
		return phase == CallPhase::STARTED || phase == CallPhase::RINGING || phase == CallPhase::SPEAKING;
	}

	inline bool is_dialing(CallPhase phase) {
		return phase == CallPhase::STARTED || phase == CallPhase::RINGING;
	}
}