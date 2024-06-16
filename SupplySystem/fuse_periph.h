//
// Created by independent-variable on 6/16/2024.
//

#pragma once
#include "./periph.h"

namespace supply {

	inline void init_fuse_pins() {
		GPIO_InitTypeDef io_conf;
		// detection of short circuit(over current)
		enable_periph_clock(DET_12V_SC_PORT);
		io_conf.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		io_conf.GPIO_Pin = DET_12V_SC_PIN;
		io_conf.GPIO_Speed = GPIO_Speed_2MHz;
		GPIO_Init(DET_12V_SC_PORT, &io_conf);
	}

	inline bool is_over_current() {
		return GPIO_ReadInputDataBit(DET_12V_SC_PORT, DET_12V_SC_PIN) == Bit_SET;
	}

	inline void detect_over_current() {
		EXTI->PR = DET_12V_SC_PIN;
		atomic_set(&EXTI->RTSR, DET_12V_SC_PIN);
		if (is_over_current()) {
			EXTI->SWIER = DET_12V_SC_PIN;
		}
	}

	inline void ignore_over_current() {
		atomic_clear(&EXTI->RTSR, ~(uint32_t)DET_12V_SC_PIN);
		EXTI->PR = DET_12V_SC_PIN;
	}
}