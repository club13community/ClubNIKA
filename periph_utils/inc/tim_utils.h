//
// Created by independent-variable on 3/15/2024.
//

#pragma once
#include "stm32f10x.h"

namespace utils {
	uint32_t get_int_clock_frequency(TIM_TypeDef * tim);
}
