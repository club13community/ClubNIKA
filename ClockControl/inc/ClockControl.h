/*
 * ClockControl.h
 *
 *  Created on: 24 бер. 2020 р.
 *      Author: MaxCm
 */

#pragma once
#include <stdint.h>
#include "stm32f10x.h"

namespace clocks {
	enum class Generator : uint8_t {HSE, HSI};
	void init();
	uint32_t get_freq(TIM_TypeDef * tim);
}
