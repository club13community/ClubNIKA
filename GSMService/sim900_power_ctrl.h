//
// Created by independent-variable on 3/20/2024.
//

#pragma once
#include <stdint.h>

namespace sim900 {
	void init_power_ctrl();
	void turn_on(void (* callback)(bool));
	void turn_off(void (* callback)());
	bool is_turned_on();
	bool is_turned_off();
	bool handle_power_ctrl_message(char * message, uint16_t length);
}