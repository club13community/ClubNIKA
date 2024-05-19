//
// Created by independent-variable on 3/20/2024.
//

#pragma once
#include <stdint.h>
#include "FreeRTOS.h"
#include "./RxBuffer.h"
#include "./config.h"

namespace sim900 {
	void init_uart_ctrl();
	void send(const char * command, uint16_t length);
	void send(const char * command, uint16_t length, BaseType_t (* callback)());
	bool is_sent();

	typedef RxBuffer<RX_BUFFER_LENGTH_pow2> rx_buffer_t;
	extern rx_buffer_t rx_buffer;
}