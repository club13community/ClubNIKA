//
// Created by independent-variable on 5/26/2024.
//
#include "sim900.h"
#include "sim900_callbacks.h"
#include "./sms.h"
#include "./state.h"
#include "./execution.h"

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
		executed();
	};

	sim900::send_sms(phone, text, sent);
	return future_result().sms_sent;
}

void gsm::delete_incoming_sms() {
	using namespace sim900;

	static constexpr auto deleted = [](Result res) {
		if (res != Result::OK && res != Result::ERROR) {
			schedule_reboot();
		}
		executed(AsyncTask::DELETE_INCOMING_SMS);
	};

	sim900::delete_received_sms(deleted);
}

void sim900::on_sms_received(uint16_t id) {
	gsm::delete_incoming_sms_asap();
}