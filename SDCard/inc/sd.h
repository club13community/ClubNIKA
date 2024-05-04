//
// Created by independent-variable on 4/21/2024.
// Limitations:
// - does not support UC cards
// - does not support cards, which use only 1 bit data bus
// - does not support CLK speeds < 10KHz
//

#pragma once
#include "sd_errors.h"
#include "sd_callbacks.h"

namespace sd {
	void init_periph();
	void start_periph();
	void write(uint32_t block_addr, uint32_t block_count, const uint8_t * buff, DataCallback callback);
	void read(uint32_t block_addr, uint32_t block_count, uint8_t * buff, DataCallback callback);
	/* Provide implementation */
	void on_card_inserted();
	/* Provide implementation */
	void on_card_removed();
}