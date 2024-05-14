/*
 * GSMService.c
 *
 *  Created on: 3 квіт. 2020 р.
 *      Author: MaxCm
 */
#include "GSMService.h"
#include "sim900.h"

void on_call_ended(sim900::Controls ctrl) {

}

/** Invoked on incoming call. */
void on_incoming_call(sim900::Controls ctrl) {

}

void gsm::init_periph() {
	sim900::init_periph();
}

void gsm::start() {
	sim900::start();
}
