//
// Created by independent-variable on 5/26/2024.
//
#include "./sms_tasks.h"
#include "./sim900.h"
#include "./state.h"
#include "./service_tasks.h"
#include "./tasks.h"

bool gsm::send_sms(const char * text, const char * phone) {
	using namespace sim900;

	static constexpr auto sent = [](uint16_t id, Result res) {
		if (res == Result::OK) {
			future_result({.sms_sent = true});
		} else {
			if (res != Result::ERROR) {
				schedule_reboot();
			}
			future_result({.sms_sent = false});
		}
		execute_scheduled();
	};

	sim900::send_sms(phone, text, sent);
	return future_result().sms_sent;
}