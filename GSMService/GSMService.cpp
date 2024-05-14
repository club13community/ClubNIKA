/*
 * GSMService.c
 *
 *  Created on: 3 квіт. 2020 р.
 *      Author: MaxCm
 */
#include "GSMService.h"
#include "sim900.h"

void on_call_ended() {

}

/** Invoked on incoming call. */
void on_incoming_call() {

}

void gsm::init_periph() {
	sim900::init_periph();
}

void gsm::start() {
	sim900::start();
}

using namespace gsm;

/*static Controls real_api = {
		.call = call,
		.accept_call = accept_call,
		.reject_call = reject_call,
		.end_call = end_call,
		.send_sms = send_sms,
		.delete_all_sms = delete_all_sms,
		.get_signal_strength = get_signal_strength,
		.get_registration = get_registration
};*/

static Controls stub_api = {
		.call = [](const char *, DialingHandler) {return false;},
		.accept_call = [](Handler) {return false;},
		.reject_call = [](Handler handler) {return false;},
		.end_call = [](Handler) {return false;},
		.send_sms = [](const char *, const char *, ResultHandler) {return false;},
		.delete_all_sms = [](ResultHandler) {return false;},
		.get_signal_strength = [](SignalHandler) {return false;},
		.get_registration = [](RegistrationHandler) {return false;}
};

Controls & gsm::get_ctrl() {
	/*while (xSemaphoreTake(ctrl_mutex, portMAX_DELAY) == pdFALSE);
	return real_api;*/
	return stub_api;
}

Controls & gsm::try_get_ctrl() {
	//return xSemaphoreTake(ctrl_mutex, 0) == pdTRUE ? real_api : stub_api;
	return stub_api;
}
