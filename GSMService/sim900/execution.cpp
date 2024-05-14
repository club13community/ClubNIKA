//
// Created by independent-variable on 5/14/2024.
//
#include "./execution.h"
#include "FreeRTOS.h"
#include "task.h"

namespace sim900 {
	tx_buffer_t tx_buffer;
	rx_buffer_t rx_buffer;
	volatile ResponseHandler response_handler;
}

void sim900::start_execution() {
	response_handler = nullptr;
}

void sim900::send_with_timeout(const char * cmd, uint16_t len, uint32_t deadline_ms, void (* timeout_elapsed)()) {

}

void sim900::start_timeout(uint32_t time_ms, void (* timeout_elapsed)()) {

}

void sim900::stop_timeout() {

}

void sim900::response_received() {

}