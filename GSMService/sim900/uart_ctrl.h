//
// Created by independent-variable on 3/20/2024.
//

#pragma once
#include "./config.h"
#include "./RxBuffer.h"

namespace sim900 {
	void init_uart_ctrl();
	void send(const char * command, uint16_t length);
}