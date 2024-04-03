//
// Created by independent-variable on 4/1/2024.
//

#pragma once
#include <stdint.h>

namespace supply_system {
	inline void process_battery_measurement(uint16_t value) {
		uint16_t voltage = value * 6; // battery voltage in mV
	}
}
