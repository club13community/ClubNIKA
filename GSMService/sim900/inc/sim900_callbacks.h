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
	void on_call_end(CallEnd end);
	/** Invoked when interlocutor presses key during a call. */
	void on_key_pressed(char key);
	void on_timestamp(rtc::Timestamp & timestamp);
	void on_sms_received(uint16_t id);
}