//
// Created by independent-variable on 4/21/2024.
// Driver for basic operations with SD card
// Limitations:
// - does not support UC cards
// - does not support cards, which use only 1 bit data bus
// - does not support CLK speeds < 10KHz
//

#pragma once
#include "sd_errors.h"
#include "sd_callbacks.h"

namespace sd {
	void init_driver();
	void start_driver();
	bool is_card_present();
	uint32_t get_block_count();
	uint32_t get_capacity_kb();
	uint32_t get_capacity_mb();
	uint32_t get_capacity_gb();
	bool is_write_protected();
	void write(uint32_t block_addr, uint32_t block_count, const uint8_t * buff, DataCallback callback);
	void read(uint32_t block_addr, uint32_t block_count, uint8_t * buff, DataCallback callback);
	constexpr uint32_t block_len = 512U;
	constexpr uint16_t block_len_pwr2 = 9;
}