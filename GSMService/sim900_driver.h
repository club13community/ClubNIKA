//
// Created by independent-variable on 3/17/2024.
//

#pragma once

namespace sim900 {
	void config_peripherals();
	void turn_on(void (* callback)(bool));
	void turn_off(void (* callback)());
	bool is_turned_on();
	bool is_turned_off();
	void handle_uart_interrupt();
}