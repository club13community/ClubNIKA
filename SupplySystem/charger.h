//
// Created by independent-variable on 6/16/2024.
//

#pragma once
#include <stdint.h>

namespace supply {
	void init_charger();
	void enable_charging();
	void enable_charging_if(volatile uint8_t * allowed);
	void disable_charging();
}