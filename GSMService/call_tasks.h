//
// Created by independent-variable on 5/24/2024.
//

#pragma once
#include "GSMService.h"
#include "sim900.h"
#include "settings.h"

namespace gsm {
	enum class CallPhase {
		RINGING,
		/** Interlocutor picked up a phone. */
		SPEAKING,
		/** No ongoing calls(all previous are ended). */
		ENDED
	};

	extern volatile CallPhase actual_call_state;
	extern volatile CallPhase handled_call_state;
	extern volatile Direction call_direction;
	extern char phone_number[MAX_PHONE_LENGTH + 1];
	/** Key pressed during phone call. */
	extern char pressed_key;

	/** @returns true if incoming call is accepted(now you may speak). */
	bool accept_call();
	void end_call();
	Dialing call(const char *number);
	/** Use during after GSM module turned off(there should be no call-related callbacks).
	 * Will emulate ending of any ongoing call. */
	void terminate_calls();

	inline bool one_of(CallPhase val, CallPhase var1, CallPhase var2) {
		return val == var1 || val == var2;
	}
}