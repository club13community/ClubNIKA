//
// Created by independent-variable on 6/13/2024.
//

#pragma once
#include "./config.h"
#include "stm32f10x.h"
#include "periph_allocation.h"
#include "rcc_utils.h"
#include "tim_utils.h"
#include "concurrent_utils.h"

#define TIMER			SUPPLY_SYSTEM_TIMER

// Connects battery to supply system(IMPORTANT to disconnect while charging)
#define EN_BAT_PORT		GPIOE
#define EN_BAT_PIN		GPIO_Pin_7
// Enables battery charger
#define CHRG_BAT_PORT	GPIOB
#define CHRG_BAT_PIN	GPIO_Pin_2

// Enables 12V supply source(channel 1 on sch.)
#define EN_12V_SRC_PORT	GPIOB
#define EN_12V_SRC_PIN	GPIO_Pin_14
// Enables siren(channel 2 on sch.)
#define EN_SIREN_PORT	GPIOE
#define EN_SIREN_PIN	GPIO_Pin_15
// Detects short circuit in ext. 12V and siren lines
#define DET_12V_SC_PORT	GPIOA
#define DET_12V_SC_PIN	GPIO_Pin_8
#define DET_12V_SC_PortSource	GPIO_PortSourceGPIOA
#define DET_12V_SC_PinSource	GPIO_PinSource8

// Detects if socket voltage is present
#define DET_SOCK_PORT	GPIOA
#define DET_SOCK_PIN	GPIO_Pin_9
#define DET_SOCK_PortSource		GPIO_PortSourceGPIOA
#define DET_SOCK_PinSource		GPIO_PinSource9

/** 8 - ext. 12V short circuit detection, 9 - socket detection*/
#define EXTI_IRQn		EXTI9_5_IRQn
#define TIMER_IRQn		TIM2_IRQn

namespace supply {
	inline void init_timer() {
		uint32_t clk_int_MHz = get_int_clock_frequency(TIMER) / 1'000'000U;
		enable_periph_clock(TIMER);
		TIM_TimeBaseInitTypeDef tim_conf = {0};
		tim_conf.TIM_CounterMode = TIM_CounterMode_Up;
		tim_conf.TIM_Period = 0xFFFFU;
		tim_conf.TIM_Prescaler = TIMER_TICK_us * clk_int_MHz - 1;
		tim_conf.TIM_RepetitionCounter = 0;
		TIM_TimeBaseInit(TIMER, &tim_conf);

		NVIC_InitTypeDef nvic_conf;
		nvic_conf.NVIC_IRQChannel = TIMER_IRQn;
		nvic_conf.NVIC_IRQChannelPreemptionPriority = SUP_SYS_IRQ_PRIORITY;
		nvic_conf.NVIC_IRQChannelSubPriority = 0;
		nvic_conf.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&nvic_conf);
	}

	inline void start_timer() {
		TIM_Cmd(TIMER, ENABLE);
	}

	inline void start_source_timeout(uint16_t ticks) {
		uint16_t ccr = TIMER->CNT + ticks;
		TIMER->CCR1 = ccr;
		TIMER->SR = ~TIM_SR_CC1IF;
		if (TIMER->CNT >= ccr) {
			TIMER->EGR = TIM_EGR_CC1G;
		}
		TIMER->DIER |= TIM_DIER_CC1IE;
	}

	inline void start_fuse_timeout(uint16_t ticks) {
		uint16_t ccr = TIMER->CNT + ticks;
		TIMER->CCR2 = ccr;
		TIMER->SR = ~TIM_SR_CC2IF;
		if (TIMER->CNT >= ccr) {
			TIMER->EGR = TIM_EGR_CC2G;
		}
		TIMER->DIER |= TIM_DIER_CC2IE;
	}

	inline bool is_source_timeout() {
		return TIMER->SR & TIM_SR_CC1IF;
	}

	inline bool is_fuse_timeout() {
		return TIMER->SR & TIM_SR_CC2IF;
	}

	inline void stop_source_timeout() {
		TIMER->DIER &= ~TIM_DIER_CC1IE;
		TIMER->SR = ~TIM_SR_CC1IF;
		// don't clear pending bit - there may be other sources
	}

	inline void stop_fuse_timeout() {
		TIMER->DIER &= ~TIM_DIER_CC2IE;
		TIMER->SR = ~TIM_SR_CC2IF;
		// don't clear pending bit - there may be other sources
	}

	inline void init_exti_lines() {
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
		GPIO_EXTILineConfig(DET_SOCK_PortSource, DET_SOCK_PinSource);
		GPIO_EXTILineConfig(DET_12V_SC_PortSource, DET_12V_SC_PinSource);

		NVIC_InitTypeDef nvic_conf;
		nvic_conf.NVIC_IRQChannel = EXTI_IRQn;
		nvic_conf.NVIC_IRQChannelPreemptionPriority = SUP_SYS_IRQ_PRIORITY;
		nvic_conf.NVIC_IRQChannelSubPriority = 0;
		nvic_conf.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&nvic_conf);
	}

	inline void enable_exti_lines() {
		EXTI->IMR |= DET_SOCK_PIN | DET_12V_SC_PIN;
	}

	inline bool is_source_exti() {
		return EXTI->PR & DET_SOCK_PIN;
	}

	inline bool is_fuse_exti() {
		return EXTI->PR & DET_12V_SC_PIN;
	}

	inline void init_12V_switches() {
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
		GPIO_WriteBit(EN_SIREN_PORT, EN_SIREN_PIN, Bit_SET);
	}

	inline void enable_siren_if(volatile uint8_t * allowed) {
		do {
			uint8_t may_en = __LDREXB((uint8_t *) allowed);
			if (!may_en) {
				__CLREX();
				return;
			}
		} while (__STREXW(EN_SIREN_PIN, EN_SIREN_PORT->BSRR));
	}

	inline void enable_12V() {
		GPIO_WriteBit(EN_12V_SRC_PORT, EN_12V_SRC_PIN, Bit_SET);
	}

	inline void disable_siren() {
		GPIO_WriteBit(EN_SIREN_PORT, EN_SIREN_PIN, Bit_RESET);
	}

	inline void disable_12V() {
		GPIO_WriteBit(EN_12V_SRC_PORT, EN_12V_SRC_PIN, Bit_RESET);
	}
}