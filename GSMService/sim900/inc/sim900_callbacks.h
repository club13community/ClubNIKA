//
// Created by independent-variable on 5/19/2024.
//

#pragma once
#include "rtc.h"
#include "sim900.h"

namespace sim900 {
	/** Invoked when problem with UART occurs. */
	void on_hw_malfunction();
	void on_call_update(uint8_t index, CallState state, CallDirection direction, char * number);
	/** Is invoked on every "RING". */
	void on_ring();
	void on_call_end(CallEnd end);
	/** Invoked when interlocutor presses key during a call. */
	void on_key_pressed(char key);
	/** @param timestamp local time for Ukraine(daylight shift is already applied).
	 * @param dst_shift "Daylight saving time" shift in hours. */
	void on_timestamp(rtc::DateTime & timestamp, uint8_t dst_shift);
	void on_dst_update(uint8_t dst_shift);
	void on_sms_received(uint16_t id);
}