//
// Created by independent-variable on 5/24/2024.
//

#pragma once
#include "GSMService.h"
#include "sim900.h"
#include "settings.h"

namespace gsm {
	extern volatile sim900::CallState actual_call_state;
	extern volatile sim900::CallState handled_call_state;
	extern volatile sim900::CallDirection call_direction;
	extern char phone_number[MAX_PHONE_LENGTH + 1];

	/** @returns true if incoming call is accepted(now you may speak). */
	bool accept_call();
	void end_call();
	Dialing call(char *number);
	/** Use during after GSM module turned off(there should be no call-related callbacks).
	 * Will emulate ending of any ongoing call. */
	void terminate_calls();

	inline Direction map(sim900::CallDirection dir) {
		return dir == sim900::CallDirection::INCOMING ? Direction::INCOMING : Direction::OUTGOING;
	}
}