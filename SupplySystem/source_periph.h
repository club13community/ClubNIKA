//
// Created by independent-variable on 6/16/2024.
//

#pragma once
#include "./periph.h"

namespace supply {
	inline void init_source_pins() {
		GPIO_InitTypeDef io_conf;
		// Socket voltage detection pin
		enable_periph_clock(DET_SOCK_PORT);
		io_conf.GPIO_Pin = DET_SOCK_PIN;
		io_conf.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		io_conf.GPIO_Speed = GPIO_Speed_2MHz;
		GPIO_Init(DET_SOCK_PORT, &io_conf);

		// Battery enabling pin
		enable_periph_clock(EN_BAT_PORT);
		io_conf.GPIO_Pin = EN_BAT_PIN;
		io_conf.GPIO_Mode = GPIO_Mode_Out_PP;
		io_conf.GPIO_Speed = GPIO_Speed_2MHz;
		GPIO_Init(EN_BAT_PORT, &io_conf);
	}

	inline bool is_socket_powered() {
		return GPIO_ReadInputDataBit(DET_SOCK_PORT, DET_SOCK_PIN) == Bit_SET;
	}

	/** Enables EXTI interrupt when socket is powered. Also clears EXTI flag.*/
	inline void detect_socket_powered() {
		atomic_clear(&EXTI->FTSR, ~(uint32_t)DET_SOCK_PIN);
		EXTI->PR = DET_SOCK_PIN;
		atomic_set(&EXTI->RTSR, DET_SOCK_PIN);
		if (is_socket_powered()) {
			EXTI->SWIER = DET_SOCK_PIN;
		}
	}

	inline void detect_socket_unpowered() {
		atomic_clear(&EXTI->RTSR, ~(uint32_t)DET_SOCK_PIN);
		EXTI->PR = DET_SOCK_PIN;
		atomic_set(&EXTI->FTSR, DET_SOCK_PIN);
		if (!is_socket_powered()) {
			EXTI->SWIER = DET_SOCK_PIN;
		}
	}

	inline void enable_battery() {
		GPIO_SetBits(EN_BAT_PORT, EN_BAT_PIN);
	}

	inline void disable_battery() {
		GPIO_ResetBits(EN_BAT_PORT, EN_BAT_PIN);
	}
}