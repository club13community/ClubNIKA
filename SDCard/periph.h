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

namespace sd {
	struct ClockConf {
		/** 'bypass' or 'divider bits' */
		uint32_t clkcr;
		/** in Hz, rounded(0.5Hz error)*/
		uint32_t real_freq;
	};

	/** @returns value for CLKCR to achieve desired freq. and real freq.(+/-0.5Hz)
	 * @throws std::exception if real freq. < 10KHz */
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
		if (real_freq < 10000) {
			throw std::exception();
		}
		uint32_t clkcr = scale < 2 ? SDIO_CLKCR_BYPASS : scale - 2;
		return {.clkcr = clkcr, .real_freq = real_freq};
	}

	/**
	 * @param clk_freq real CLK freq(+/- 0.5Hz)
	 * @returns power-up time: max(1ms, 74 clocks), - in units of ms
	 * @throws std::exception if clk_freq is less than 10kHz
	 */
	inline uint16_t get_power_up_time_ms(uint32_t clk_freq) {
		if (clk_freq >= 80000) {
			return 1;
		} else if (clk_freq >= 40000) {
			return 2;
		} else if (clk_freq >= 20000) {
			return 4;
		} else if (clk_freq >= 10000) {
			return 8;
		} else {
			throw std::exception();
		}
	}

	/** Sets time during which card should send/program data
	 * @param clk_freq real freq. of CLK in Hz(+/-0.5Hz) */
	inline void set_read_write_timeout(uint32_t clk_freq) {
		uint32_t clk_kHz = (2 * clk_freq + 1000) / (2 * 1000);
		uint32_t time_clks = READ_WRITE_TIME_ms * clk_kHz;
		SDIO->DTIMER = time_clks;
	}

	inline uint32_t set_slow_clk() {
		ClockConf clk_conf = get_clk_div(CLK_INIT_FREQ);
		SDIO->CLKCR = SDIO->CLKCR & ~SDIO_CLKCR_BYPASS & ~SDIO_CLKCR_CLKDIV | clk_conf.clkcr;
		return clk_conf.real_freq;
	}

	inline uint32_t set_fast_clk() {
		ClockConf clk_conf = get_clk_div(CLK_APP_FREQ);
		SDIO->CLKCR = SDIO->CLKCR & ~SDIO_CLKCR_BYPASS & ~SDIO_CLKCR_CLKDIV | clk_conf.clkcr;
		return clk_conf.real_freq;
	}

	inline void init_sdio() {
		RCC_AHBPeriphClockCmd(RCC_AHBPeriph_SDIO, ENABLE);
		SDIO->CLKCR = SDIO_CLKCR_CLKEN | get_clk_div(CLK_INIT_FREQ).clkcr;

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
		__NOP(); __NOP(); __NOP();
		__NOP(); __NOP(); __NOP();
		__NOP();
	}

	inline void dis_sdio_clk() {
		SDIO->POWER = 0;
		//and 7 HCLK periods delay
		__NOP(); __NOP(); __NOP();
		__NOP(); __NOP(); __NOP();
		__NOP();
	}

	inline bool is_sdio_clk_en() {
		return SDIO->POWER == (SDIO_POWER_PWRCTRL_1 | SDIO_POWER_PWRCTRL_0);
	}

	inline void use_1bit_dat() {
		SDIO->CLKCR = SDIO->CLKCR & ~SDIO_CLKCR_WIDBUS;
	}

	inline void use_4bits_dat() {
		SDIO->CLKCR = SDIO->CLKCR & ~SDIO_CLKCR_WIDBUS | SDIO_CLKCR_WIDBUS_0;
	}

	inline void tx_via_dma(uint32_t * buffer, uint16_t length) {
		DMA_CHANNEL->CPAR = (uint32_t)&SDIO->FIFO;
		DMA_CHANNEL->CMAR = (uint32_t)buffer;
		DMA_CHANNEL->CNDTR = length;
		DMA_CHANNEL->CCR = SD_DMA_PRIORITY | DMA_CCR1_MINC | DMA_CCR1_DIR | DMA_CCR1_EN
				| DMA_CCR1_MSIZE_1 | DMA_CCR1_PSIZE_1;
	}

	inline void rx_via_dma(uint32_t * buffer, uint16_t length) {
		DMA_CHANNEL->CPAR = (uint32_t)&SDIO->FIFO;
		DMA_CHANNEL->CMAR = (uint32_t)buffer;
		DMA_CHANNEL->CNDTR = length;
		DMA_CHANNEL->CCR = SD_DMA_PRIORITY | DMA_CCR1_MINC | DMA_CCR1_EN
				| DMA_CCR1_MSIZE_1 | DMA_CCR1_PSIZE_1;
	}

	inline void stop_dma() {
		DMA_CHANNEL->CCR = 0;
		while (DMA_CHANNEL->CCR & DMA_CCR1_EN);
	}

	inline bool is_card_inserted() {
		return GPIO_ReadInputDataBit(DETECT_PORT, DETECT_PIN) == Bit_RESET;
	}
}