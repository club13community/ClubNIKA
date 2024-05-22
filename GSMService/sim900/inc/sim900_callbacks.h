//
// Created by independent-variable on 5/19/2024.
//

#pragma once
#include "rtc.h"
#include "sim900.h"

namespace sim900 {
	void on_ring();
	void on_call_end(CallEnd end);
	void on_timestamp(rtc::Timestamp & timestamp);
	// todo void on_sms_received();
}