//
// Created by independent-variable on 5/19/2024.
//
#include "./listeners.h"
#include "sim900_callbacks.h"
#include "rtc.h"
#include "settings.h"
#include "./utils.h"
#include <stdlib.h>

bool sim900::call_state_listener(rx_buffer_t & rx) {
	if (!rx.starts_with("+CLCC:")) {
		return false;
	}

	// length of phone number + opt. "+country code" + quotes + '\0'
	// MAX_PHONE_LENGTH takes into account last '0' from country code
	constexpr uint8_t max_param_len = MAX_PHONE_LENGTH + (1 + 2) + 2 + 1;
	// parse direction
	char param[max_param_len];
	rx.get_param(1, param, 1);
	CallDirection dir = param[0] == '0' ? CallDirection::OUTGOING : CallDirection::INCOMING;
	// parse state
	rx.get_param(2, param, 1);
	CallState state;
	if (param[0] == '0') {
		state = CallState::SPEAKING;
	} else if (param[0] == '6') {
		state = CallState::ENDED;
	} else {
		// observed only 2, 3, 4
		state = CallState::RINGING;
	}
	// parse phone number(is quoted)
	uint16_t len = rx.get_param(5, param, max_param_len - 1);
	copy(param, param[1] == '+' ? 4 : 1, len - 1, param);
	on_call_update(state, dir, param);
	return true;
}

bool sim900::call_end_listener(rx_buffer_t & rx) {
	if (rx.equals("NO CARRIER")) {
		on_call_end(CallEnd::NORMAL);
		return true;
	}
	if (rx.equals("BUSY")) {
		on_call_end(CallEnd::BUSY);
		return true;
	}
	if (rx.equals("NO ANSWER")) {
		on_call_end(CallEnd::NO_ANSWER);
		return true;
	}
	return false;
}

bool sim900::timestamp_listener(rx_buffer_t & rx) {
	if (rx.is_message_corrupted()) {
		return false;
	}
	if (rx.starts_with("DST:")) {
		return true; // related to network time but is not used
	}
	if (!rx.starts_with("*PSUTTZ:")) {
		return false;
	}
	//parse "*PSUTTZ: ..."
	char param[5];
	using namespace rtc;

	rx.get_param(0, param, 4);
	uint16_t year = atoi(param);
	rx.get_param(1, param, 4);
	Month month = Month(atoi(param));
	rx.get_param(2, param, 4);
	uint8_t day = atoi(param);
	Date date = Date(year, month, day);

	rx.get_param(3, param, 4);
	uint8_t hour = atoi(param);
	rx.get_param(4, param, 4);
	uint8_t minute = atoi(param);
	rx.get_param(5, param, 4);
	uint8_t second = atoi(param);
	Time time = Time(hour, minute, second);

	Timestamp timestamp = Timestamp(date, time);
	on_timestamp(timestamp);
	return true;
}

bool sim900::ignoring_listener(rx_buffer_t & rx) {
	if (rx.is_message_corrupted()) {
		return false;
	}
	return rx.equals("RING") || rx.equals("Call Ready") || rx.starts_with("*PSNWID:") || rx.equals("+CFUN: 1");
}