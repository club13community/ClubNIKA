//
// Created by independent-variable on 5/14/2024.
//

#pragma once
#include "../uart_ctrl.h"

namespace sim900 {
	void handle_uart_interrupt();
	void handle_dma_interrupt();
}
