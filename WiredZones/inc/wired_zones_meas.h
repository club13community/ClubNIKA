//
// Created by independent-variable on 4/1/2024.
//

#pragma once
#include <stdint.h>

namespace wired_zones {
	enum class Measurement : uint8_t {NONE, SOURCING, SINKING};

	/** Should be invoked before selecting measurement */
	void process_measurement(uint16_t value);

	/** Connects zone sensor to ADC */
	Measurement select_measurement();
}