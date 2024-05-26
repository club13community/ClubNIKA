//
// Created by independent-variable on 5/21/2024.
//

#pragma once
#include "sim900.h"
#include <stdint.h>

namespace gsm {
	extern volatile bool powered;
	extern volatile sim900::CardStatus card_status;
	extern volatile sim900::Registration registration;
	extern volatile uint8_t signal_strength;

	void init_service_tasks();
	void turn_on();
	void turn_off();
	void check_module_state();
}