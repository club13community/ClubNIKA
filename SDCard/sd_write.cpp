//
// Created by independent-variable on 4/28/2024.
//
#include "sd.h"
#include "cmd_execution.h"
#include "data_exchange.h"
#include "sd_info_private.h"
#include "sd_read_write_state.h"
#include "periph.h"
#include "sd_ctrl.h"

using namespace sd;

#define ATTEMPTS	2

static inline uint32_t to_addr(uint32_t block) {
	return sd::hcs == sd::CapacitySupport::SC ? block << sd::block_len_pwr2 : block;
}

static void write_block(uint32_t block_addr);
static void write_cmd_sent(CSR_t csr, Error error);
static void card_state_received(CSR_t csr, Error error);
static void data_sent(Error error);
static void prog_status_received(CSR_t csr, Error error);
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

static void write_block(uint32_t block_addr) {
	// send command
	rw::cmd_attempts = ATTEMPTS;
	exe_cmd24(to_addr(block_addr), write_cmd_sent);
}

static void write_cmd_sent(CSR_t csr, Error error) {
	if (error == Error::NONE) {
		// send data
		send((uint8_t *) rw::buffer, data_sent);
	} else if (error == Error::CMD_CRC_ERROR) {
		// card did not recognise command - send again
		if (--rw::cmd_attempts > 0) {
			write_failed(error);
		} else {
			write_failed(Error::CMD_CRC_ERROR);
		}
	} else if (error == Error::RESP_CRC_ERROR) {
		// not clear if card recognized command - check state of a card
		rw::status_attempts = ATTEMPTS;
		exe_cmd13(RCA, card_state_received);
	} else {
		write_failed(error);
	}
}

static void card_state_received(CSR_t csr, Error error) {
	if (error == Error::NONE || is_from_response(error)) {
		// no problems with communication, response is valid
		State state = csr.state();
		if (state == State::TRAN) {
			// card did not recognize command and stayed in 'tran' state - send command again
			if (--rw::cmd_attempts > 0) {
				exe_cmd24(to_addr(rw::block_address), write_cmd_sent);
			} else {
				write_failed(Error::CMD_CRC_ERROR);
			}
		} else if (state == State::RCV) {
			// card recognized command and is already in 'receive data' state - send data
			send((uint8_t *) rw::buffer, data_sent);
		} else {
			// unexpected state
			write_failed(Error::GENERAL_ERROR);
		}
	} else if (error == Error::RESP_CRC_ERROR) {
		// request state again
		if (--rw::status_attempts > 0) {
			exe_cmd13(RCA, card_state_received);
		} else {
			write_failed(Error::RESP_CRC_ERROR);
		}
	} else {
		write_failed(error);
	}
}

static void data_sent(Error error) {
	if (error == Error::NONE) {
		// check status of programming
		rw::status_attempts = READ_WRITE_TIME_ms;
		exe_cmd13(RCA, prog_status_received);
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

static void prog_status_received(CSR_t csr, Error error) {
	auto send_cmd13 = []() {
		exe_cmd13(RCA, prog_status_received);
	};
	if (error == Error::NONE) {
		if (csr.state() == State::TRAN) {
			// block programmed
			block_write_done();
		} else {
			// check again in 1ms
			if (--rw::status_attempts > 0) {
				TIMER.invoke_in_ms(1, send_cmd13);
			} else {
				write_failed(Error::DATA_TIMEOUT);
			}
		}
	} else if (error == Error::CMD_CRC_ERROR || error == Error::RESP_CRC_ERROR) {
		// check again in 1 ms
		if (--rw::status_attempts > 0) {
			TIMER.invoke_in_ms(1, send_cmd13);
		} else {
			write_failed(Error::DATA_TIMEOUT);
		}
	} else {
		write_failed(error);
	}
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


