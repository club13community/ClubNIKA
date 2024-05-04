//
// Created by independent-variable on 4/27/2024.
//

#pragma once
#include <stdint.h>
#include "sd_callbacks.h"

namespace sd {
	void send(uint8_t * buff, Callback callback);
	void receive(uint8_t * buff, Callback callback);
	void cancel_receive();
	void handle_data_irq(uint32_t sta);
}