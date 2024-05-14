//
// Created by independent-variable on 5/11/2024.
//

#pragma once
#include <stdint.h>

namespace sim900 {
	/** Interrupts should be enabled(uses timer to discharge decoupling cap.)*/
	void init_periph();
	void start();
}

/** Invoked when initiator ends incoming call before accepting/rejecting by driver
 * or when ongoing call is ended by interlocutor(not by driver).
 * Is not invoked when call is ended/rejected by driver. */
void on_call_ended();

/** Invoked on incoming call. */
void on_incoming_call();

// todo void on_sms_received();