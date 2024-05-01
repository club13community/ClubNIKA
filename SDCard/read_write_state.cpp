//
// Created by independent-variable on 4/30/2024.
//
#include <stdint.h>
#include "sd_callbacks.h"

namespace sd::rw {
	volatile DataCallback on_done;
	volatile uint32_t block_address;
	uint8_t * volatile buffer;
	volatile uint32_t target_blocks, done_blocks;
	/** Attempts to read/write data block. */
	volatile uint8_t data_attempts;
	/** Attempts of commands while reading/writing data (except 'send status' command) */
	volatile uint8_t cmd_attempts;
	/** Attempts of 'send status' commands while reading/writing data (except 'send status' command) */
	volatile uint16_t status_attempts;
}
