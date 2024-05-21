//
// Created by independent-variable on 5/21/2024.
//
#include "./state.h"
#include "sim900.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

namespace gsm {
	void (* volatile on_incoming_call)(char *, gsm::Controls &) = nullptr;

	void (* volatile on_call_ended)(gsm::Controls &) = nullptr;

	volatile sim900::CardStatus card_status = sim900::CardStatus::ERROR;
	volatile sim900::Registration registration = sim900::Registration::ONGOING;
	volatile uint8_t signal_strength = 0;

	volatile sim900::CallState call_state = sim900::CallState::ENDED;
	char phone_number[MAX_PHONE_LENGTH + 1];

	SemaphoreHandle_t ctrl_mutex;


	void reboot_state() {
		using namespace sim900;
		card_status = CardStatus::ERROR;
		registration = Registration::ONGOING;
		signal_strength = 0;
		call_state = CallState::ENDED;
	}
}