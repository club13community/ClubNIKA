//
// Created by independent-variable on 6/16/2024.
//
#include "./charger.h"
#include "./periph.h"

namespace supply {
	volatile bool charging;
}

void supply::init_charger() {
	enable_periph_clock(CHRG_BAT_PORT);
	GPIO_InitTypeDef io_conf;
	io_conf.GPIO_Pin = CHRG_BAT_PIN;
	io_conf.GPIO_Mode = GPIO_Mode_Out_PP;
	io_conf.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(CHRG_BAT_PORT, &io_conf);

	charging = false;
}

void supply::enable_charging() {
	GPIO_WriteBit(CHRG_BAT_PORT, CHRG_BAT_PIN, Bit_SET);
}

void supply::enable_charging_if(volatile uint8_t * allowed) {
	do {
		uint8_t may_en = __LDREXB((uint8_t *)allowed);
		if (!may_en) {
			__CLREX();
			return;
		}
	} while (__STREXW(CHRG_BAT_PIN, (uint32_t *)&CHRG_BAT_PORT->BSRR));
}

void supply::disable_charging() {
	GPIO_WriteBit(CHRG_BAT_PORT, CHRG_BAT_PIN, Bit_RESET);
}