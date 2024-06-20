//
// Created by independent-variable on 5/19/2024.
//
#include "./listeners.h"
#include "sim900_callbacks.h"
#include "rtc.h"
#include <stdlib.h>

bool sim900::pressed_key_listener(rx_buffer_t & rx) {
	if (rx.is_message_corrupted()) {
		return false;
	}
	if (rx.starts_with("+DTMF:")) {
		char param[2];
		rx.get_param(0, param, 1);
		on_key_pressed(param[0]);
		return true;
	} else {
		return false;
	}
}

bool sim900::timestamp_listener(rx_buffer_t & rx) {
	if (rx.is_message_corrupted()) {
		return false;
	}
	if (!rx.starts_with("*PSUTTZ:")) {
		return false;
	}
	//parse "*PSUTTZ: ..."
	char param[5];
	using namespace rtc;

	Date date;
	rx.get_param(0, param, 4);
	date.year= atoi(param);
	rx.get_param(1, param, 4);
	date.month = Month(atoi(param));
	rx.get_param(2, param, 4);
	date.day = atoi(param);

	Time time;
	rx.get_param(3, param, 4);
	time.hour = atoi(param);
	rx.get_param(4, param, 4);
	time.minute = atoi(param);
	rx.get_param(5, param, 4);
	time.second = atoi(param);
	rx.get_param(7, param, 4);
	uint8_t dst = atoi(param);

	DateTime timestamp = {.date = date, .time = time};
	add_hours(2U + dst, timestamp); // convert UTC to local Ukrainian
	on_timestamp(timestamp, dst);
	return true;
}

bool sim900::dst_listener(rx_buffer_t & rx) {
	if (rx.is_message_corrupted()) {
		return false;
	}
	if (!rx.starts_with("DST:")) {
		return false;
	}
	// parse "DST: ..."
	char param[2];
	rx.get_param(0, param, 1);
	uint8_t dst = atoi(param);
	on_dst_update(dst);
	return true;
}

bool sim900::ignoring_listener(rx_buffer_t & rx) {
	if (rx.is_message_corrupted()) {
		return false;
	}
	return rx.equals("Call Ready") || rx.starts_with("*PSNWID:")
		|| rx.starts_with("+CFUN:") || rx.starts_with("+CPIN:");
}