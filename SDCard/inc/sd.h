//
// Created by independent-variable on 4/21/2024.
// Limitations:
// - does not support UC cards
// - does not support cards, which use only 1 bit data bus
//

#pragma once
#include "sd_errors.h"
#include "sd_callbacks.h"

namespace sd {
	void init_periph();
	void init_card(Callback callback);
	void write(uint32_t block_addr, uint32_t block_count, uint8_t * buff, DataCallback callback);
	void read(uint32_t block_addr, uint32_t block_count, uint8_t * buff, DataCallback callback);
}