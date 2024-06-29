//
// Created by independent-variable on 3/23/2024.
//

#pragma once
#include <stdint.h>
#include "stm32f10x.h"
#include "rcc_utils.h"

#define MUTE_PORT	GPIOE
#define MUTE_PIN	GPIO_Pin_10

#define SPEAKER_INPUT_SELECT_PORT	GPIOE
#define SPEAKER_INPUT_SELECT_PIN	GPIO_Pin_12

#define SIM900_MIC_SELECT_PORT		GPIOE
#define SIM900_MIC_SELECT_PIN		GPIO_Pin_11

namespace player {

	inline void mute_speaker() {
		GPIO_SetBits(MUTE_PORT, MUTE_PIN);
	}

	inline void unmute_speaker() {
		GPIO_ResetBits(MUTE_PORT, MUTE_PIN);
	}

	inline void connect_speaker_to_dac() {
		GPIO_ResetBits(SPEAKER_INPUT_SELECT_PORT, SPEAKER_INPUT_SELECT_PIN);
	}

	inline void connect_speaker_to_sim900() {
		GPIO_SetBits(SPEAKER_INPUT_SELECT_PORT, SPEAKER_INPUT_SELECT_PIN);
	}

	inline void connect_dac_to_sim900() {
		GPIO_SetBits(SIM900_MIC_SELECT_PORT, SIM900_MIC_SELECT_PIN);
	}

	inline void disconnect_dac_from_sim900() {
		GPIO_ResetBits(SIM900_MIC_SELECT_PORT, SIM900_MIC_SELECT_PIN);
	}

	inline void init_mux_pins() {
		GPIO_InitTypeDef io_conf = {0};
		io_conf.GPIO_Mode = GPIO_Mode_Out_PP;
		io_conf.GPIO_Speed = GPIO_Speed_2MHz;

		enable_periph_clock(MUTE_PORT);
		mute_speaker();
		io_conf.GPIO_Pin = MUTE_PIN;
		GPIO_Init(MUTE_PORT, &io_conf);

		enable_periph_clock(SPEAKER_INPUT_SELECT_PORT);
		connect_speaker_to_dac();
		io_conf.GPIO_Pin = SPEAKER_INPUT_SELECT_PIN;
		GPIO_Init(SPEAKER_INPUT_SELECT_PORT, &io_conf);

		enable_periph_clock(SIM900_MIC_SELECT_PORT);
		disconnect_dac_from_sim900();
		io_conf.GPIO_Pin = SIM900_MIC_SELECT_PIN;
		GPIO_Init(SIM900_MIC_SELECT_PORT, &io_conf);
	}
}