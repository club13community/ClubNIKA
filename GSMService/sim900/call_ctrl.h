//
// Created by independent-variable on 5/14/2024.
//

#pragma once
#include "sim900.h"

namespace sim900 {
	/** @param phone '\0' ended phone number without leading '+' and country code. */
	bool call(const char * phone, DialingHandler handler);
	/** If no incoming call - do not invoke handler(if there was incoming call,
	 * but ended before accepting - appropriate callback is invoked) */
	bool accept_call(Handler handler);
	/** If no incoming call - do not invoke handler(if there was incoming call,
	 * but ended before rejecting - appropriate callback is invoked) */
	bool reject_call(Handler handler);
	/** If no ongoing call - do not invoke handler(if there was ongoing call,
	 * but ended before ending - appropriate callback is invoked) */
	bool end_call(Handler handler);
}