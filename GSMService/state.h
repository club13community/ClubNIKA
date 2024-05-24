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
	/** Phases of handling a call. Almost everything what changes phase of handling should do it like
	 * "set DIALING if FREE now". */ // todo change this
	//enum class CallHandling : uint8_t {FREE, DIALING, SPEAKING, ENDING};

	union FutureResult {
		int16_t sms_id;
		Dialing dialing;
		bool call_ended;
		bool call_accepted;
	};

	extern void (* volatile  on_incoming_call)(char *);
	extern void (* volatile on_call_ended)();

	extern volatile sim900::CardStatus card_status;
	extern volatile sim900::Registration registration;
	extern volatile uint8_t signal_strength;

	extern volatile sim900::CallState actual_call_state;
	extern volatile sim900::CallState handled_call_state;
	extern volatile sim900::CallDirection call_direction;
	extern char phone_number[MAX_PHONE_LENGTH + 1];

	/** Guards access to controlling methods. */
	extern volatile SemaphoreHandle_t ctrl_mutex;
	/** Ensures, order of call-related callbacks and methods. */
	extern volatile SemaphoreHandle_t call_mutex;

	void init_state();
	void reboot_state();

	void future_result(FutureResult result);
	FutureResult future_result();

	inline void take_ctrl_mutex() {
		while (xSemaphoreTake(ctrl_mutex, portMAX_DELAY) == pdFALSE);
	}

	inline void give_ctrl_mutex() {
		xSemaphoreGive(ctrl_mutex);
	}

	inline void take_call_mutex() {
		while (xSemaphoreTake(ctrl_mutex, portMAX_DELAY) == pdFALSE);
	}

	inline void give_call_mutex() {
		xSemaphoreGive(ctrl_mutex);
	}

	inline bool one_of(sim900::CallState val, sim900::CallState var1, sim900::CallState var2) {
		return val == var1 || val == var2;
	}

	inline void safe_on_incoming_call(char * number) {
		void (* callback_now)(char *) = on_incoming_call;
		if (callback_now != nullptr) {
			callback_now(number);
		}
	}

	inline void safe_on_call_ended() {
		void (* callback_now)() = on_call_ended;
		if (callback_now != nullptr) {
			callback_now();
		}
	}
}