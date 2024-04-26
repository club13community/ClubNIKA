//
// Created by independent-variable on 4/26/2024.
//

#pragma once
#include <stdint.h>
#include "sd_enums.h"

namespace sd {
	extern volatile CapacitySupport hcs;
	extern volatile uint16_t RCA;
	void parse_CID(uint8_t * buff);
	bool parse_CSD(uint8_t * buff);
}