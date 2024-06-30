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
	uint32_t get_freq(SPI_TypeDef * spi);
	/** @returns freq. of clock for external interface. */
	uint32_t get_freq(SDIO_TypeDef * sdio);
}
