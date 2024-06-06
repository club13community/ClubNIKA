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
	if (rx.starts_with("DST:")) {
		return true; // related to network time but is not used
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

	DateTime timestamp = {.date = date, .time = time};
	on_timestamp(timestamp);
	return true;
}

bool sim900::ignoring_listener(rx_buffer_t & rx) {
	if (rx.is_message_corrupted()) {
		return false;
	}
	return rx.equals("RING") || rx.equals("Call Ready") || rx.starts_with("*PSNWID:") || rx.equals("+CFUN: 1");
}