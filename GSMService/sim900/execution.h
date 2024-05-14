//
// Created by independent-variable on 5/14/2024.
//

#pragma once
#include "./uart_ctrl.h"

namespace sim900 {
	typedef char tx_buffer_t[TX_BUFFER_LENGTH];
	typedef RxBuffer<RX_BUFFER_LENGTH_pow2> rx_buffer_t;
	typedef bool (* ResponseHandler)(rx_buffer_t & rx);

	extern tx_buffer_t tx_buffer;
	extern rx_buffer_t rx_buffer;
	/** Handler of SIM900 responses */
	extern volatile ResponseHandler response_handler;

	/** Creates thread, timer, etc. which are needed for command execution. */
	void start_execution();
	/** Invoke when new message from SIM900 arrives */
	void response_received();

	void send_with_timeout(const char * cmd, uint16_t len, uint32_t deadline_ms, void (* timeout_elapsed)());

	void start_timeout(uint32_t time_ms, void (* timeout_elapsed)());
	void stop_timeout();
}