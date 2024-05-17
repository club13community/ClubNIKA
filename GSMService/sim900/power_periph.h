//
// Created by independent-variable on 3/17/2024.
//

#pragma once
#include "stm32f10x.h"
#include "timing.h"

#define TIMER (timing::coarse_timer3)

#define PWR_KEY_PORT	GPIOD
#define PWR_KEY_PIN		GPIO_Pin_4
#define VBAT_SW_PORT	GPIOD
#define VBAT_SW_PIN		GPIO_Pin_1
#define VBAT_DISCHARGE_PORT	GPIOD
#define VBAT_DISCHARGE_PIN	GPIO_Pin_0

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
}