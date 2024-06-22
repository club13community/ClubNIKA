//
// Created by independent-variable on 4/30/2024.
//
#include <stdint.h>
#include "./sd_read_write.h"
#include "sd_callbacks.h"
#include "./cmd_execution.h"

namespace sd::rw {
	volatile DataCallback on_done;
	volatile uint32_t block_address;
	uint8_t * volatile buffer;
	volatile uint32_t target_blocks, done_blocks;
	/** Attempts to read/write data block. */
	volatile uint8_t data_attempts;
	/** Attempts of read/write commands. */
	volatile uint8_t cmd_attempts;
}

void sd::rw::get_state(void (* callback)(State state, Error error), uint16_t max_attempts) {
	static void (* volatile handler)(State, Error);
	static volatile uint16_t attempts;

	handler = callback;
	attempts = max_attempts;

	static constexpr CSR_Consumer on_status = [](CSR_t csr, Error err) {
		if (err == Error::NONE || is_from_response(err)) {
			handler(csr.state(), Error::NONE); // state retrieved
		} else if (--attempts > 0) {
			exe_cmd13(RCA, on_status);
		} else {
			handler(State::IDLE, err); // any state, as it will be ignored
		}
	};

	exe_cmd13(RCA, on_status);
}