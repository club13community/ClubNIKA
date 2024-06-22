//
// Created by independent-variable on 4/28/2024.
//
#include "./sd_read_write.h"
#include "./cmd_execution.h"
#include "./data_exchange.h"
#include "./periph.h"
#include "./sd_ctrl.h"

using namespace sd;

#define ATTEMPTS	2

static inline uint32_t to_addr(uint32_t block) {
	return sd::hcs == sd::CapacitySupport::SC ? block << sd::block_len_pwr2 : block;
}

static void write_block(uint32_t block_addr);
static void data_sent(Error error);
static void block_write_done();
static void write_done();
static void write_failed(Error error);

void sd::write(uint32_t block_addr, uint32_t block_count, const uint8_t * buff, DataCallback callback) {
	Error error = start_operation();
	if (error != Error::NONE) {
		callback(0, error);
	}
	rw::block_address = block_addr;
	rw::target_blocks = block_count;
	rw::done_blocks = 0;
	rw::buffer = (uint8_t *)buff;
	rw::on_done = callback;
	if (block_count > 0) {
		rw::data_attempts = ATTEMPTS;
		write_block(block_addr);
	} else {
		write_done();
	}
}

static void write_cmd_sent(CSR_t csr, Error error);

static void write_block(uint32_t block_addr) {
	// send command
	rw::cmd_attempts = ATTEMPTS;
	exe_cmd24(to_addr(block_addr), write_cmd_sent);
}

static inline void retry_write_cmd(Error cause) {
	if (--rw::cmd_attempts > 0) {
		exe_cmd24(to_addr(rw::block_address), write_cmd_sent);
	} else {
		write_failed(cause);
	}
}

static void card_state_received(State state, Error error); // will be needed if response CRC fails

static void write_cmd_sent(CSR_t csr, Error error) {
	if (error == Error::NONE) {
		// send data
		send((uint8_t *) rw::buffer, data_sent);
	} else if (error == Error::CMD_CRC_ERROR || error == Error::NO_RESPONSE) {
		// card did not recognise command - send again
		retry_write_cmd(error);
	} else if (error == Error::RESP_CRC_ERROR) {
		// not clear if card recognized command - check state of a card
		rw::get_state(card_state_received, ATTEMPTS);
	} else {
		write_failed(error);
	}
}

static void card_state_received(State state, Error error) {
	if (error != Error::NONE) {
		write_failed(error);
		return;
	}

	if (state == State::TRAN) {
		// card did not recognize command and stayed in 'tran' state - send command again
		retry_write_cmd(Error::CMD_CRC_ERROR);
	} else if (state == State::RCV) {
		// card recognized command and is already in 'receive data' state - send data
		send((uint8_t *) rw::buffer, data_sent);
	} else {
		// unexpected state
		write_failed(Error::GENERAL_ERROR);
	}
}

static void wait_programmed();

static void data_sent(Error error) {
	if (error == Error::NONE) {
		wait_programmed();
	} else if (error == Error::DATA_CRC_ERROR) {
		// try to write the same block again
		if (--rw::data_attempts > 0) {
			write_block(rw::block_address);
		} else {
			write_failed(Error::DATA_CRC_ERROR);
		}
	} else {
		write_failed(error);
	}
}

static void wait_programmed() {
	static volatile uint16_t attempts;

	static constexpr void (* state_received)(State, Error) = [](State state, Error error) {
		if (error != Error::NONE) {
			write_failed(error);
			return;
		}

		if (state == State::TRAN) {
			block_write_done();
		} else if (--attempts > 0) {
			TIMER.invoke_in_ms(1, []() {
				rw::get_state(state_received, ATTEMPTS);
			});
		} else {
			write_failed(Error::DATA_TIMEOUT);
		}
	};

	attempts = READ_WRITE_TIME_ms;
	rw::get_state(state_received, ATTEMPTS);
}

static void block_write_done() {
	if (++rw::done_blocks == rw::target_blocks) {
		write_done();
	} else {
		uint32_t addr = ++rw::block_address;
		rw::buffer += block_len;
		rw::data_attempts = ATTEMPTS;
		write_block(addr);
	}
}

static void write_done() {
	end_operation(Error::NONE);
	rw::on_done(rw::done_blocks, Error::NONE);
}

static void write_failed(Error error) {
	end_operation(error);
	rw::on_done(rw::done_blocks, error);
}


