//
// Created by independent-variable on 5/14/2024.
// Everything what is needed for command execution
//

#pragma once
#include "./config.h"
#include "./RxBuffer.h"
#include "FreeRTOS.h"

namespace sim900 {
	typedef char tx_buffer_t[TX_BUFFER_LENGTH];
	typedef RxBuffer<RX_BUFFER_LENGTH_pow2> rx_buffer_t;
	typedef bool (* ResponseHandler)(rx_buffer_t &);

	extern tx_buffer_t tx_buffer;
	extern rx_buffer_t rx_buffer;

	/** Invoke when new message from SIM900 arrives */
	void response_received();

	/** Invoke at the beginning of command execution */
	void begin_command(ResponseHandler handler);
	void change_listener(ResponseHandler handler);
	/** Invoke before calling result handler */
	void end_command();

	/** Sends and starts timeout till handled response. Can not be simultaneously used with other timeout functions. */
	void send_with_timeout(const char * cmd, uint16_t len, uint32_t deadline_ms, void (* timeout_elapsed)());
	void send_with_timeout(const char * cmd, uint16_t len, void (* message_sent)(),
						   uint32_t deadline_ms, void (* timeout_elapsed)());
	/** Starts timeout till handled response. Can not be simultaneously used with other timeout functions.*/
	void start_response_timeout(uint32_t deadline_ms, void (* timeout_elapsed)());

	void start_timeout(uint32_t delay_ms, void (* callback)());
	void stop_timeout();
}