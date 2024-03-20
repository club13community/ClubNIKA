//
// Created by independent-variable on 3/17/2024.
//

#pragma once
#include "config.h"

namespace sim900 {
	inline void press_pwr_key() {
		GPIO_WriteBit(PWR_KEY_PORT, PWR_KEY_PIN, Bit_RESET);
	}

	inline void release_pwr_key() {
		GPIO_WriteBit(PWR_KEY_PORT, PWR_KEY_PIN, Bit_SET);
	}

	/** Connect 4V supply to VBAT */
	inline void enable_vbat() {
		GPIO_WriteBit(VBAT_SW_PORT, VBAT_SW_PIN, Bit_SET);
	}

	inline void disable_vbat() {
		GPIO_WriteBit(VBAT_SW_PORT, VBAT_SW_PIN, Bit_RESET);
	}

	/** Short VBAT to discharge decoupling caps. */
	inline void short_vbat() {
		GPIO_WriteBit(VBAT_DISCHARGE_PORT, VBAT_DISCHARGE_PIN, Bit_SET);

	}

	inline void open_vbat() {
		GPIO_WriteBit(VBAT_DISCHARGE_PORT, VBAT_DISCHARGE_PIN, Bit_RESET);
	}

	inline void activate_uart() {
		GPIO_InitTypeDef io_conf = {.GPIO_Pin = TX_PIN, .GPIO_Speed = GPIO_Speed_2MHz, .GPIO_Mode = GPIO_Mode_AF_PP};
		GPIO_Init(TX_PORT, &io_conf);
		USART_Cmd(UART, ENABLE);
	}

	inline void suspend_uart() {
		USART_Cmd(UART, DISABLE);
		GPIO_InitTypeDef io_conf = {.GPIO_Pin = TX_PIN, .GPIO_Mode = GPIO_Mode_IN_FLOATING};
		GPIO_Init(TX_PORT, &io_conf);
	}

}