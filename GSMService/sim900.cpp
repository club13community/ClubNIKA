//
// Created by independent-variable on 5/11/2024.
//
#include "sim900.h"
#include "FreeRTOS.h"
#include "semphr.h"

using namespace sim900;

static SemaphoreHandle_t ctrl_mutex;

void sim900::init_driver() {
	static StaticSemaphore_t mutex_ctrl;
	ctrl_mutex = xSemaphoreCreateBinaryStatic(&mutex_ctrl);
}

static bool call(const char * phone, DialingHandler handler) {
	return true;
}

static bool accept_call(Handler handler) {
	return true;
}

static bool reject_call(Handler handler) {
	return true;
}

static bool end_call(Handler handler) {
	return true;
}

static bool send_sms(const char * message, const char * phone, ResultHandler handler) {
	return true;
}

static bool delete_all_sms(ResultHandler handler) {
	return true;
}

static bool get_signal_strength(SignalHandler handler) {
	return true;
}

static bool get_registration(RegistrationHandler handler) {
	return true;
}

static Controls real_api = {
	.call = call,
	.accept_call = accept_call,
	.reject_call = reject_call,
	.end_call = end_call,
	.send_sms = send_sms,
	.delete_all_sms = delete_all_sms,
	.get_signal_strength = get_signal_strength,
	.get_registration = get_registration
};

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

Controls & sim900::get_ctrl() {
	while (xSemaphoreTake(ctrl_mutex, portMAX_DELAY) == pdFALSE);
	return real_api;
}

Controls & sim900::try_get_ctrl() {
	return xSemaphoreTake(ctrl_mutex, 0) == pdTRUE ? real_api : stub_api;
}