//
// Created by independent-variable on 4/21/2024.
// Limitations:
// - does not support UC cards
// - does not support cards, which use only 1 bit data bus
//

#pragma once
#include "sd_errors.h"

namespace sd {
	void init_periph();
	void init_card(void (* callback)(Error));
}