#include "GSMService.h"
#include "sim900.h"
#include "sim900_callbacks.h"
#include "rtc.h"
#include "./state.h"
#include "./service_tasks.h"
#include "./callback_handling.h"
#include "./call_tasks.h"

void gsm::init_periph() {
	sim900::init_periph();
}

void gsm::start() {
	init_state();
	init_callback_handling();
	init_service_tasks();

	sim900::start();
}

uint8_t gsm::get_signal_strength() {
	using namespace sim900;
	return card_status == CardStatus::READY && registration == Registration::DONE ? signal_strength : 0;
}

void gsm::set_on_incoming_call(void (* callback)(char *)) {
	on_incoming_call = callback;
}

void gsm::set_on_call_dialed(void (* callback)(Direction direction)) {
	on_call_dialed = callback;
}

void gsm::set_on_call_ended(void (* callback)()) {
	on_call_ended = callback;
}

gsm::Controls & gsm::get_ctrl() {
	while (xSemaphoreTake(ctrl_mutex, portMAX_DELAY) == pdFALSE);
	return Controls::inst;
}

void sim900::on_timestamp(rtc::Timestamp & timestamp) {

}

gsm::Dialing gsm::Controls::call(const char * phone) {
	return gsm::call(phone);
}

void gsm::Controls::end_call() {
	gsm::end_call();
}

bool gsm::Controls::accept_call() {
	return gsm::accept_call();
}

int16_t gsm::Controls::send_sms(const char * phone) {
	return -1;
}