//
// Created by independent-variable on 4/21/2024.
//

#pragma once
#include "stm32f10x.h"
#include "periph_allocation.h"
#include "rcc_utils.h"
#include "dma_utils.h"
#include "timing.h"
#include <exception>
#include "config.h"

#define TIMER		(timing::coarse_timer1)

#define DETECT_PORT	GPIOA
#define DETECT_PIN	GPIO_Pin_15
#define DAT_PORT	GPIOC
#define DAT_PINS	(GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11)
#define CMD_PORT	GPIOD
#define CMD_PIN		GPIO_Pin_2
#define CLK_PORT	GPIOC
#define CLK_PIN		GPIO_Pin_12

#define DMA				DMA2
#define DMA_CHANNEL		SD_DMA_CHANNEL
#define DMA_ISR_TCIF	DMA_ISR_TCIF4
#define DMA_IFCR_CTCIF	DMA_IFCR_CTCIF4

namespace sd {
	struct ClockConf {
		/** 'bypass' or 'divider bits' */
		uint32_t clkcr;
		/** in Hz, rounded(0.5Hz error)*/
		uint32_t real_freq;
	};

	/** @returns value for CLKCR to achieve desired freq. */
	inline ClockConf get_clk_div(uint32_t target_freq) {
		RCC_ClocksTypeDef clk_conf;
		RCC_GetClocksFreq(&clk_conf);
		uint32_t input = clk_conf.HCLK_Frequency;
		uint32_t scale = input / target_freq;
		uint32_t real_freq = target_freq;
		if (input % target_freq) {
			scale += 1;
			real_freq = (2 * input + scale)/(2 * scale);
		}
		const uint32_t max_div = (1U << 8) - 1 + 2;
		if (scale > max_div) {
			throw std::exception();
		}
		uint32_t clkcr = scale < 2 ? SDIO_CLKCR_BYPASS : scale - 2;
		return {.clkcr = clkcr, .real_freq = real_freq};
	}

	inline uint32_t set_slow_clk() {
		ClockConf clk_conf = get_clk_div(CLK_INIT_FREQ);
		SDIO->CLKCR = clk_conf.clkcr | SDIO_CLKCR_CLKEN;
		return clk_conf.real_freq;
	}

	inline uint32_t set_fast_clk() {
		ClockConf clk_conf = get_clk_div(CLK_APP_FREQ);
		SDIO->CLKCR = clk_conf.clkcr | SDIO_CLKCR_CLKEN;
		return clk_conf.real_freq;
	}

	inline void init_sdio() {
		RCC_AHBPeriphClockCmd(RCC_AHBPeriph_SDIO, ENABLE);
		set_slow_clk();
		SDIO->DCTRL = SDIO_DCTRL_SDIOEN;

		NVIC_InitTypeDef nvic_conf;
		nvic_conf.NVIC_IRQChannel = SDIO_IRQn;
		nvic_conf.NVIC_IRQChannelPreemptionPriority = SD_IRQ_PRIORITY;
		nvic_conf.NVIC_IRQChannelSubPriority = 0;
		nvic_conf.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&nvic_conf);
	}

	inline void init_sdio_pins() {
		GPIO_InitTypeDef io_conf;
		// CLK
		enable_periph_clock(CLK_PORT);
		io_conf.GPIO_Pin = CLK_PIN;
		io_conf.GPIO_Mode = GPIO_Mode_AF_PP;
		io_conf.GPIO_Speed = GPIO_Speed_10MHz;
		GPIO_Init(CLK_PORT, &io_conf);
		// DATx
		enable_periph_clock(DAT_PORT);
		io_conf.GPIO_Pin = DAT_PINS;
		io_conf.GPIO_Mode = GPIO_Mode_AF_PP;
		io_conf.GPIO_Speed = GPIO_Speed_10MHz;
		GPIO_Init(DAT_PORT, &io_conf);
		// CMD
		enable_periph_clock(CMD_PORT);
		io_conf.GPIO_Pin = CMD_PIN;
		io_conf.GPIO_Mode = GPIO_Mode_AF_PP;
		io_conf.GPIO_Speed = GPIO_Speed_10MHz;
		GPIO_Init(CMD_PORT, &io_conf);
	}

	inline void init_detect_pin() {
		enable_periph_clock(DETECT_PORT);
		GPIO_InitTypeDef io_conf;
		io_conf.GPIO_Pin = DETECT_PIN;
		io_conf.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		io_conf.GPIO_Speed = GPIO_Speed_2MHz;
		GPIO_Init(DETECT_PORT, &io_conf);
	}

	inline void init_dma() {
		enable_periph_clock(DMA);
	}

	inline void en_sdio_clk() {
		SDIO->POWER = SDIO_POWER_PWRCTRL_1 | SDIO_POWER_PWRCTRL_0;
		//and 7 HCLK periods delay
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
	}

	inline void dis_sdio_clk() {
		SDIO->POWER = 0;
		//and 7 HCLK periods delay
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
	}

	inline bool is_sdio_clk_en() {
		return SDIO->POWER == (SDIO_POWER_PWRCTRL_1 | SDIO_POWER_PWRCTRL_0);
	}

	inline void use_4bits_dat() {
		SDIO->CLKCR = SDIO->CLKCR & ~SDIO_CLKCR_WIDBUS | SDIO_CLKCR_WIDBUS_0;
	}

	inline void tx_via_dma(uint8_t * buffer, uint16_t length) {
		DMA_CHANNEL->CPAR = (uint32_t)&SDIO->FIFO;
		DMA_CHANNEL->CMAR = (uint32_t)buffer;
		DMA_CHANNEL->CNDTR = length;
		DMA_CHANNEL->CCR = SD_DMA_PRIORITY | DMA_CCR1_MINC | DMA_CCR1_DIR | DMA_CCR1_EN;
	}

	inline void rx_via_dma(uint8_t * buffer, uint16_t length) {
		DMA_CHANNEL->CPAR = (uint32_t)&SDIO->FIFO;
		DMA_CHANNEL->CMAR = (uint32_t)buffer;
		DMA_CHANNEL->CNDTR = length;
		DMA_CHANNEL->CCR = SD_DMA_PRIORITY | DMA_CCR1_MINC | DMA_CCR1_EN;
	}

	inline bool is_dma_tc() {
		return DMA->ISR & DMA_ISR_TCIF;
	}

	inline void clear_dma_tc() {
		DMA->IFCR = DMA_IFCR_CTCIF;
	}

	inline bool is_card_inserted() {
		return GPIO_ReadInputDataBit(DETECT_PORT, DETECT_PIN) == Bit_RESET;
	}
}