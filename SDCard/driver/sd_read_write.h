//
// Created by independent-variable on 4/30/2024.
// Common for reading and writing
//

#pragma once
#include <stdint.h>
#include "sd_callbacks.h"
#include "sd_driver.h"
#include "./sd_info.h"

namespace sd::rw {
	extern volatile DataCallback on_done;
	extern volatile uint32_t block_address;
	extern uint8_t * volatile buffer;
	extern volatile uint32_t target_blocks, done_blocks;
	extern volatile uint8_t data_attempts;
	extern volatile uint8_t cmd_attempts;
	/** @param callback will get Error::NONE if state was retrieved. */
	void get_state(void (* callback)(State state, Error error), uint16_t max_attempts);

	inline uint32_t to_addr(uint32_t block) {
		return sd::hcs == sd::CapacitySupport::SC ? block << sd::block_len_pwr2 : block;
	}
}