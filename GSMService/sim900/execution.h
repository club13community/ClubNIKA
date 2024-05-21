//
// Created by independent-variable on 5/14/2024.
// Everything what is needed for command execution
//

#pragma once
#include "./uart_ctrl.h"
#include "./config.h"
#include "FreeRTOS.h"

namespace sim900 {
	typedef char tx_buffer_t[TX_BUFFER_LENGTH];
	typedef bool (* ResponseListener)(rx_buffer_t &);

	extern tx_buffer_t tx_buffer;

	/** Invoke at the beginning of command execution */
	void begin_command(ResponseListener listener);
	/** Invoke before calling result handler */
	void end_command();

	/** Sends and starts timeout till executed response. Can not be simultaneously used with other timeout functions. */
	void send_with_timeout(const char * cmd, uint16_t len, uint32_t deadline_ms, void (* timeout_elapsed)());
	void send_with_timeout(const char * cmd, uint16_t len, void (* message_sent)(),
						   uint32_t deadline_ms, void (* timeout_elapsed)());
	/** Starts timeout till executed response. Can not be simultaneously used with other timeout functions.*/
	void start_response_timeout(uint32_t deadline_ms, void (* timeout_elapsed)());

	void start_timeout(uint32_t delay_ms, void (* timeout_elapsed)());
	void stop_timeout();
}