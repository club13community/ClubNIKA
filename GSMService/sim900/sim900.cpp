//
// Created by independent-variable on 5/11/2024.
//
#include "sim900.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "./execution.h"
#include "./power_ctrl.h"
#include "./uart_ctrl.h"
#include "./call_ctrl.h"
#include "./sms_ctrl.h"
#include "./status_ctrl.h"

using namespace sim900;

static SemaphoreHandle_t ctrl_mutex;

void sim900::init_periph() {
	init_power_ctrl();
	init_uart_ctrl();
}

void sim900::start() {
	static StaticSemaphore_t mutex_ctrl;
	ctrl_mutex = xSemaphoreCreateBinaryStatic(&mutex_ctrl);
	start_execution();
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