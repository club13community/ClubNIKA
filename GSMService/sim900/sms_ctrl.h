//
// Created by independent-variable on 5/26/2024.
//

#pragma once
#include "./uart_ctrl.h"

namespace sim900 {
	bool incoming_sms_listener(rx_buffer_t & rx);
}