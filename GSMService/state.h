//
// Created by independent-variable on 5/21/2024.
//

#pragma once
#include "GSMService.h"
#include "sim900.h"
#include "settings.h"
#include "FreeRTOS.h"
#include "semphr.h"

namespace gsm {

	union FutureResult {
		Dialing dialing;
		bool call_ended;
		bool call_accepted;
		bool sms_sent;
	};

	extern void (* volatile  on_incoming_call)(char *);
	extern void (* volatile on_call_dialed)(Direction);
	extern void (* volatile on_key_pressed)(char);
	extern void (* volatile on_call_ended)();

	extern volatile bool powered;
	extern volatile sim900::CardStatus card_status;
	extern volatile sim900::Registration registration;
	extern volatile uint8_t signal_strength;

	/** Guards access to controlling methods. */
	extern volatile SemaphoreHandle_t ctrl_mutex;

	void init_state();

	inline void reset_connection_info() {
		gsm::card_status = sim900::CardStatus::ERROR;
		gsm::registration = sim900::Registration::ONGOING;
		gsm::signal_strength = 0;
	}

	void future_result(FutureResult result);

	FutureResult future_result();

	inline void safe_on_incoming_call(char * number) {
		void (* callback_now)(char *) = on_incoming_call;
		if (callback_now != nullptr) {
			callback_now(number);
		}
	}

	inline void safe_on_call_dialed(Direction direction) {
		void (* callback_now)(Direction) = on_call_dialed;
		if (callback_now != nullptr) {
			callback_now(direction);
		}
	}

	inline void safe_on_key_pressed(char key) {
		void (* callback_now)(char) = on_key_pressed;
		if (callback_now != nullptr) {
			callback_now(key);
		}
	}

	inline void safe_on_call_ended() {
		void (* callback_now)() = on_call_ended;
		if (callback_now != nullptr) {
			callback_now();
		}
	}
}