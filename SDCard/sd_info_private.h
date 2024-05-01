//
// Created by independent-variable on 4/26/2024.
//

#pragma once
#include <stdint.h>
#include "sd_types.h"

namespace sd {
	extern volatile CapacitySupport hcs;
	extern volatile uint16_t RCA;
	void parse_CID(uint8_t * buff);
	bool parse_CSD(uint8_t * buff);
	constexpr uint32_t block_len = 512U;
	constexpr uint16_t block_len_pwr2 = 9;
}