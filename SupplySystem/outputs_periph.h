//
// Created by independent-variable on 6/17/2024.
//

#pragma once
#include "./periph.h"

namespace supply {
	inline void init_output_switches() {
		GPIO_InitTypeDef io_conf;
		// enablers of siren and 12V source
		io_conf.GPIO_Mode = GPIO_Mode_Out_PP;
		io_conf.GPIO_Speed = GPIO_Speed_2MHz;

		enable_periph_clock(EN_12V_SRC_PORT);
		io_conf.GPIO_Pin = EN_12V_SRC_PIN;
		GPIO_Init(EN_12V_SRC_PORT, &io_conf);

		enable_periph_clock(EN_SIREN_PORT);
		io_conf.GPIO_Pin = EN_SIREN_PIN;
		GPIO_Init(EN_SIREN_PORT, &io_conf);
	}

	inline void enable_siren() {
		GPIO_SetBits(EN_SIREN_PORT, EN_SIREN_PIN);
	}

	inline bool enable_siren_excl() {
		return __STREXW(EN_SIREN_PIN, (uint32_t *)&EN_SIREN_PORT->BSRR);
	}

	inline void enable_12V() {
		GPIO_SetBits(EN_12V_SRC_PORT, EN_12V_SRC_PIN);
	}

	inline void disable_siren() {
		GPIO_ResetBits(EN_SIREN_PORT, EN_SIREN_PIN);
	}

	inline void disable_12V() {
		GPIO_ResetBits(EN_12V_SRC_PORT, EN_12V_SRC_PIN);
	}
}