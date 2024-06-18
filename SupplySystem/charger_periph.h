//
// Created by independent-variable on 6/17/2024.
//

#pragma once
#include "./periph.h"

namespace supply {
	inline void init_charger_pins() {
		enable_periph_clock(CHRG_BAT_PORT);
		GPIO_InitTypeDef io_conf;
		io_conf.GPIO_Pin = CHRG_BAT_PIN;
		io_conf.GPIO_Mode = GPIO_Mode_Out_PP;
		io_conf.GPIO_Speed = GPIO_Speed_2MHz;
		GPIO_Init(CHRG_BAT_PORT, &io_conf);
	}

	inline void enable_charging() {
		GPIO_SetBits(CHRG_BAT_PORT, CHRG_BAT_PIN);
	}

	inline bool enable_charging_excl() {
		return __STREXW(CHRG_BAT_PIN, (uint32_t *)&CHRG_BAT_PORT->BSRR);
	}

	inline void disable_charging() {
		GPIO_ResetBits(CHRG_BAT_PORT, CHRG_BAT_PIN);
	}
}