//
// Created by independent-variable on 3/30/2024.
//

#pragma once
#include "stm32f10x.h"

#define SEL_PORT	GPIOE
#define SEL0_PIN	GPIO_Pin_0
#define SEL1_PIN	GPIO_Pin_1
#define SEL2_PIN	GPIO_Pin_2

#define DIR_PORT	GPIOB
#define DIR_PIN		GPIO_Pin_9

namespace wired_zones {
	/** @param ch - 3 LSB bits */
	inline void select_channel(uint8_t ch) {
		GPIO_SetBits(SEL_PORT, 0x07U & ch);
		GPIO_ResetBits(SEL_PORT, 0x07U & ~ch);
	}

	inline void measure_sink_current() {
		GPIO_SetBits(DIR_PORT, DIR_PIN);
	}

	inline void measure_source_current() {
		GPIO_ResetBits(DIR_PORT, DIR_PIN);
	}

	inline void init_ch_sel_pins() {
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
		select_channel(0);
		GPIO_InitTypeDef io_conf;
		io_conf.GPIO_Pin = SEL0_PIN | SEL1_PIN | SEL2_PIN;
		io_conf.GPIO_Mode = GPIO_Mode_Out_PP;
		io_conf.GPIO_Speed = GPIO_Speed_2MHz;
		GPIO_Init(SEL_PORT, &io_conf);
	}

	inline void init_dir_pin() {
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
		measure_sink_current();
		GPIO_InitTypeDef io_conf;
		io_conf.GPIO_Pin = DIR_PIN;
		io_conf.GPIO_Mode = GPIO_Mode_Out_PP;
		io_conf.GPIO_Speed = GPIO_Speed_2MHz;
		GPIO_Init(DIR_PORT, &io_conf);
	}
}
