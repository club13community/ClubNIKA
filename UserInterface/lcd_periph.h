//
// Created by independent-variable on 3/25/2024.
//

#pragma once
#include "stm32f10x.h"
#include "timing.h"

/*
 * RS - PE3
 * RW - PE4
 * E - PE5
 * LED - PE6
 * DB7..4 - PC3..0
 */
#define RS_RW_E_LED_PORT GPIOE
#define RS_PIN		GPIO_Pin_3
#define RW_PIN		GPIO_Pin_4
#define	E_PIN		GPIO_Pin_5
#define LED_PIN		GPIO_Pin_6

#define DB_PORT	GPIOC
#define DB_PINS	(GPIO_Pin_3 | GPIO_Pin_2 | GPIO_Pin_1 | GPIO_Pin_0)

#define TIMER	(timing::fine_timer2)
// time(in us) while E is 1
#define E_PULSE	10U
// time(in us) while DB does not change after E falling edge
#define DB_HOLD	5U

namespace lcd {

	inline void rw_read() {
		GPIO_SetBits(RS_RW_E_LED_PORT, RW_PIN);
	}

	inline void rw_write() {
		GPIO_ResetBits(RS_RW_E_LED_PORT, RW_PIN);
	}

	inline void rs_data() {
		GPIO_SetBits(RS_RW_E_LED_PORT, RS_PIN);
	}

	inline void rs_command() {
		GPIO_ResetBits(RS_RW_E_LED_PORT, RS_PIN);
	}

	inline void e_low() {
		GPIO_ResetBits(RS_RW_E_LED_PORT, E_PIN);
	}

	inline void e_high() {
		GPIO_SetBits(RS_RW_E_LED_PORT, E_PIN);
	}

	inline void led_on() {
		GPIO_SetBits(RS_RW_E_LED_PORT, LED_PIN);
	}

	inline void led_off() {
		GPIO_ResetBits(RS_RW_E_LED_PORT, LED_PIN);
	}

	inline void db_as_input() {
		GPIO_InitTypeDef pinInitStruct;
		pinInitStruct.GPIO_Pin = DB_PINS;
		pinInitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		pinInitStruct.GPIO_Speed = GPIO_Speed_2MHz; // any
		GPIO_Init(DB_PORT, &pinInitStruct);
	}

	inline void db_as_output() {
		GPIO_InitTypeDef pinInitStruct;
		pinInitStruct.GPIO_Pin = DB_PINS;
		pinInitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
		pinInitStruct.GPIO_Speed = GPIO_Speed_2MHz;
		GPIO_Init(DB_PORT, &pinInitStruct);
	}

	inline uint8_t db_get() {
		return GPIO_ReadInputData(DB_PORT) & 0x000FU;
	}

	inline void db_set(uint8_t nibble) {
		GPIO_SetBits(DB_PORT, 0x000F & (uint16_t)nibble);
		GPIO_ResetBits(DB_PORT, 0x000F & ~(uint16_t)nibble);
	}

	/** inits for reading */
	inline void init_pins() {
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

		rs_command(); // any is ok
		rw_read();
		e_low();
		led_off();

		GPIO_InitTypeDef pinInitStruct;
		pinInitStruct.GPIO_Speed=GPIO_Speed_2MHz;
		pinInitStruct.GPIO_Mode=GPIO_Mode_Out_PP;
		pinInitStruct.GPIO_Pin = RS_PIN | RW_PIN | E_PIN | LED_PIN;
		GPIO_Init(RS_RW_E_LED_PORT, &pinInitStruct);

		db_as_input();
	}

	void inline switch_to_write() {
		rw_write();
		TIMER.wait_us(10);
		db_as_output();
	}

	void inline switch_to_read() {
		db_as_input();
		TIMER.wait_us(10);
		rw_read();
	}

	void inline write_nibble(uint8_t val) {
		db_set(val); // MSB nibble is masked inside

		e_high();
		TIMER.wait_us(E_PULSE);
		e_low();
		TIMER.wait_us(DB_HOLD);
	}

	void inline write(uint8_t val) {
		write_nibble(val >> 4);
		write_nibble(val); // MSB nibble is masked inside
	}

	inline uint8_t next_ddram_address(uint8_t addr) {
		if (addr == 0x27) {
			return 0x40;
		} else if (addr == 0x67) {
			return 0x00;
		} else {
			return addr + 1;
		}
	}
}