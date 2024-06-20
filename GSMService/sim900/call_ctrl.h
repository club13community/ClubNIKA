//
// Created by independent-variable on 5/25/2024.
//

#pragma once
#include "./uart_ctrl.h"

namespace sim900 {
	bool call_state_listener(rx_buffer_t & rx);
	bool ring_listener(rx_buffer_t & rx);
	bool call_end_listener(rx_buffer_t & rx);
}