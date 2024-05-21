//
// Created by independent-variable on 5/21/2024.
//

#pragma once
#include "GSMService.h"
#include "sim900.h"
#include "settings.h"

#include "semphr.h"
#include "task.h"
#include "timers.h"

namespace gsm {
	extern void (* volatile  on_incoming_call)(char *, gsm::Controls &);
	extern void (* volatile on_call_ended)(gsm::Controls &);

	extern volatile sim900::CardStatus card_status;
	extern volatile sim900::Registration registration;
	extern volatile uint8_t signal_strength;

	extern volatile sim900::CallState call_state;
	extern char phone_number[MAX_PHONE_LENGTH + 1];

	extern SemaphoreHandle_t ctrl_mutex;

	void reboot_state();
}