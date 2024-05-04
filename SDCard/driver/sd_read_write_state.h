//
// Created by independent-variable on 4/30/2024.
//

#pragma once
#include <stdint.h>
#include "sd_callbacks.h"

namespace sd::rw {
	extern volatile DataCallback on_done;
	extern volatile uint32_t block_address;
	extern uint8_t * volatile buffer;
	extern volatile uint32_t target_blocks, done_blocks;
	extern volatile uint8_t data_attempts;
	extern volatile uint16_t cmd_attempts;
	extern volatile uint8_t status_attempts;
}