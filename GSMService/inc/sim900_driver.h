//
// Created by independent-variable on 3/17/2024.
//

#pragma once
#include <stdint.h>

namespace sim900 {
	void config_peripherals();
	void turn_on(void (* callback)(bool));
	void turn_off(void (* callback)());
	bool is_turned_on();
	bool is_turned_off();
	void send(char *, uint16_t, void (*)());
	void handle_uart_interrupt();
	void handle_dma_interrupt();
}