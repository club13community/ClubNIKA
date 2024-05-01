//
// Created by independent-variable on 4/30/2024.
//
#include "sd.h"
#include "cmd_execution.h"
#include "data_exchange.h"
#include "sd_info_private.h"
#include "read_write_state.h"
#include "stm32f10x.h"

using namespace sd;

#define ATTEMPTS	2
static volatile bool checking_state, waiting_data;
static volatile Error data_error;

static inline uint32_t to_addr(uint32_t block) {
	return sd::hcs == CapacitySupport::SC ? block << block_len_pwr2 : block;
}

static inline uint32_t dis_irq() {
	uint32_t mask = __get_PRIMASK();
	__set_PRIMASK(1U);
	return mask;
}

static inline void en_irq(uint32_t mask) {
	__set_PRIMASK(mask);
}

static void read_block(uint32_t block_addr, uint8_t * buff);
static void read_cmd_sent(CSR_t csr, Error error);
static void state_received(CSR_t csr, Error error);
static void data_received(Error error);
static void handle_received_data(Error error);
static void block_read_done();
static void read_done();
static void read_failed(Error error);

void sd::read(uint32_t block_addr, uint32_t block_count, uint8_t * buff, sd::DataCallback callback) {
	if (block_count > 0) {
		rw::block_address = block_addr;
		rw::target_blocks = block_count;
		rw::done_blocks = 0;
		rw::buffer = buff;
		rw::on_done = callback;

		rw::data_attempts = ATTEMPTS;
		read_block(block_addr, buff);
	} else {
		callback(0, Error::NONE);
	}
}

static inline void read_block(uint32_t block_addr, uint8_t * buff) {
	rw::cmd_attempts = ATTEMPTS; // attempts for CMD17
	waiting_data = true;
	checking_state = false;
	uint32_t mask = dis_irq();
	receive(buff, data_received);
	exe_cmd17(to_addr(block_addr), read_cmd_sent);
	en_irq(mask);
}

static void read_cmd_sent(CSR_t csr, Error error) {
	if (error == Error::NONE) {
		// command was accepted, wait for data
		// nothing to do
	} else if (error == Error::CMD_CRC_ERROR) {
		// command 'read block' was not recognized, send it again
		// note: data timeout is ticking
		if (--rw::cmd_attempts > 0) {
			exe_cmd17(to_addr(rw::block_address), read_cmd_sent);
		} else {
			read_failed(Error::CMD_CRC_ERROR);
		}
	} else if (error == Error::RESP_CRC_ERROR) {
		// not sure if command was accepted - request card's state
		// note: data timeout is ticking, card may start sending data
		rw::status_attempts = ATTEMPTS;
		checking_state = true;
		exe_cmd13(RCA, state_received);
	} else {
		// retry will not help - read failed
		read_failed(error);
	}
}

// Invoked only if response to 'read block' failed CRC
static void state_received(CSR_t csr, Error error) {
	if (!waiting_data) {
		// block is already received - no need to check anything
		// checking_state is not valuable now
		handle_received_data(data_error);
	} else if (error == Error::NONE || is_from_response(error)) {
		// no problem with communication - value is correct, there will be no reasons to request state again
		checking_state = false;
		State state = csr.state();
		if (state == State::TRAN) {
			// card did not start to send data - send 'read block' command again
			// note: data timeout is ticking
			if (--rw::cmd_attempts > 0) {
				exe_cmd17(to_addr(rw::block_address), read_cmd_sent);
			} else {
				read_failed(Error::CMD_CRC_ERROR);
			}
		} else if (state == State::DATA) {
			// card is sending data
			// nothing to do
		} else {
			// card recognized command but did not move to 'sending data'(moved to unexpected state instead)
			read_failed(Error::GENERAL_ERROR);
		}
	} else if (error == Error::RESP_CRC_ERROR) {
		// request state again
		if (--rw::status_attempts > 0) {
			exe_cmd13(RCA, state_received);
		} else {
			// checking_state is not valuable now
			read_failed(Error::RESP_CRC_ERROR);
		}
	} else {
		// not retryable error with communication
		// checking_state is not valuable now
		read_failed(error);
	}
}

static void data_received(Error error) {
	waiting_data = false;
	if (checking_state) {
		// command path is busy, handling of received data will be done while checking state
		data_error = error;
	} else {
		// command path is free, handle reception here
		handle_received_data(error);
	}
}

static void handle_received_data(Error error) {
	if (error == Error::NONE) {
		block_read_done();
	} else if (error == Error::DATA_CRC_ERROR) {
		// read block again
		if (--rw::data_attempts > 0) {
			read_block(rw::block_address, rw::buffer);
		} else {
			read_failed(Error::DATA_CRC_ERROR);
		}
	} else {
		// retry will not help - read failed
		read_failed(error);
	}
}

static void block_read_done() {
	if (++rw::done_blocks == rw::target_blocks) {
		read_done();
	} else {
		uint32_t addr = ++rw::block_address;
		uint8_t * buff = rw::buffer += block_len;
		rw::data_attempts = ATTEMPTS;
		read_block(addr, buff);
	}
}

static void read_done() {
	rw::on_done(rw::done_blocks, Error::NONE);
}

static void read_failed(Error error) {
	cancel_receive();
	rw::on_done(rw::done_blocks, error);
}

