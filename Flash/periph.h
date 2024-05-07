//
// Created by independent-variable on 3/26/2024.
//

#pragma once
#include "stm32f10x.h"
#include "periph_allocation.h"
#include "./config.h"
#include <exception>

#define SPI			FLASH_SPI
#define TX_DMA		FLASH_TX_DMA_CHANNEL
#define RX_DMA		FLASH_RX_DMA_CHANNEL
#define SPI_PORT	GPIOA
#define SCK_PIN		GPIO_Pin_5
#define MISO_PIN	GPIO_Pin_6
#define MOSI_PIN	GPIO_Pin_7

#define CS_PORT		GPIOC
#define CS_PIN		GPIO_Pin_4

#define TX_DMA_CCR	(FLASH_TX_DMA_PRIORITY | DMA_CCR1_DIR)
#define RX_DMA_CCR	(FLASH_RX_DMA_PRIORITY | DMA_CCR1_TCIE)

namespace flash {
	extern volatile uint32_t pclk_freq;

	inline void cs_select() {
		GPIO_ResetBits(CS_PORT, CS_PIN);
	}

	inline void cs_deselect() {
		GPIO_SetBits(CS_PORT, CS_PIN);
	}

	inline void init_cs() {
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
		cs_deselect();
		GPIO_InitTypeDef io_conf;
		io_conf.GPIO_Pin = CS_PIN;
		io_conf.GPIO_Mode = GPIO_Mode_Out_PP;
		io_conf.GPIO_Speed = GPIO_Speed_10MHz;
		GPIO_Init(CS_PORT, &io_conf);
	}

	inline void init_mosi_miso_sck() {
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

		GPIO_InitTypeDef io_conf = {0};
		io_conf.GPIO_Pin = MOSI_PIN | SCK_PIN;
		io_conf.GPIO_Mode = GPIO_Mode_AF_PP;
		io_conf.GPIO_Speed = GPIO_Speed_10MHz;
		GPIO_Init(SPI_PORT, &io_conf);

		io_conf.GPIO_Pin = MISO_PIN;
		io_conf.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		GPIO_Init(SPI_PORT, &io_conf);
	}

	inline uint16_t select_prescaler(uint32_t pclk_freq) {
		if (pclk_freq >= SPI_FREQ * 2) {
			return SPI_BaudRatePrescaler_2;
		}
		if (pclk_freq >= SPI_FREQ * 4) {
			return SPI_BaudRatePrescaler_4;
		}
		if (pclk_freq >= SPI_FREQ * 8) {
			return SPI_BaudRatePrescaler_8;
		}
		if (pclk_freq >= SPI_FREQ * 16) {
			return SPI_BaudRatePrescaler_16;
		}
		if (pclk_freq >= SPI_FREQ * 32) {
			return SPI_BaudRatePrescaler_32;
		}
		if (pclk_freq >= SPI_FREQ * 64) {
			return SPI_BaudRatePrescaler_64;
		}
		if (pclk_freq >= SPI_FREQ * 128) {
			return SPI_BaudRatePrescaler_128;
		}
		if (pclk_freq >= SPI_FREQ * 256) {
			return SPI_BaudRatePrescaler_256;
		}
		throw std::exception();
	}

	inline void init_spi() {
		RCC_ClocksTypeDef clocks;
		RCC_GetClocksFreq(&clocks);
		pclk_freq = clocks.PCLK2_Frequency;
		if (clocks.PCLK2_Frequency < 1'000'000U || clocks.PCLK2_Frequency > 48'000'000U) {
			throw std::exception();
		}

		RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

		SPI_InitTypeDef spi_conf = {0};
		spi_conf.SPI_Mode = SPI_Mode_Master;
		spi_conf.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
		spi_conf.SPI_DataSize = SPI_DataSize_8b;
		spi_conf.SPI_FirstBit = SPI_FirstBit_MSB;
		spi_conf.SPI_BaudRatePrescaler = select_prescaler(clocks.PCLK2_Frequency);
		spi_conf.SPI_CPOL = SPI_CPOL_High;
		spi_conf.SPI_CPHA = SPI_CPHA_2Edge;
		spi_conf.SPI_NSS = SPI_NSS_Soft;
		SPI_Init(SPI, &spi_conf);
		SPI_I2S_DMACmd(SPI, SPI_I2S_DMAReq_Rx, ENABLE);
		SPI_I2S_DMACmd(SPI, SPI_I2S_DMAReq_Tx, ENABLE);
		SPI_I2S_ITConfig(SPI, SPI_I2S_IT_ERR, ENABLE);
		SPI_Cmd(SPI, ENABLE);

		RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
		TX_DMA->CPAR = (uint32_t)&SPI->DR;
		RX_DMA->CPAR = (uint32_t)&SPI->DR;
		// the rest is configured before enabling

		// NVIC for DMA(RX TC)
		NVIC_InitTypeDef nvic_conf = {0};
		nvic_conf.NVIC_IRQChannel = DMA1_Channel2_IRQn;
		nvic_conf.NVIC_IRQChannelPreemptionPriority = FLASH_IRQ_PRIORITY;
		nvic_conf.NVIC_IRQChannelSubPriority = 0;
		nvic_conf.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&nvic_conf);

		// NVIC for SPI(RX overrun)
		nvic_conf.NVIC_IRQChannel = SPI1_IRQn;
		nvic_conf.NVIC_IRQChannelPreemptionPriority = FLASH_IRQ_PRIORITY;
		nvic_conf.NVIC_IRQChannelSubPriority = 0;
		nvic_conf.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&nvic_conf);
	}

	enum class MemInc : uint32_t {NO = 0, YES = DMA_CCR1_MINC};

	inline void enable_tx_dma(volatile uint8_t * buffer, uint16_t count, MemInc mem_inc) {
		TX_DMA->CMAR = (uint32_t)buffer;
		TX_DMA->CNDTR = count;
		TX_DMA->CCR = TX_DMA_CCR | (uint32_t)mem_inc | DMA_CCR1_EN;
	}

	inline void enable_rx_dma(volatile uint8_t * buffer, uint16_t count, MemInc mem_inc) {
		RX_DMA->CMAR = (uint32_t)buffer;
		RX_DMA->CNDTR = count;
		RX_DMA->CCR = RX_DMA_CCR | (uint32_t)mem_inc | DMA_CCR1_EN;
	}

	inline void disable_tx_dma() {
		TX_DMA->CCR = TX_DMA_CCR;
		while (TX_DMA->CCR & DMA_CCR1_EN);
	}

	inline void disable_rx_dma() {
		RX_DMA->CCR = RX_DMA_CCR;
		while (TX_DMA->CCR & DMA_CCR1_EN);
	}

	inline void clear_rx_dma_tc() {
		DMA1->IFCR = DMA_IFCR_CTCIF2;
	}

	/** Guaranties 1/10MHz delay. Allows CS to settle between 2 consequent operations */
	inline void cs_delay() {
		uint32_t pclk = pclk_freq;
		(void)SPI->CR1; // read something via APBx bus of SPI, assume that takes 1 APBx clock(didn't find in spec.)
		if (pclk <= 10'000'000U) {
			return;
		}
		(void)SPI->CR1;
		if (pclk <= 20'000'000U) {
			return;
		}
		(void)SPI->CR1;
		if (pclk <= 30'000'000U) {
			return;
		}
		(void)SPI->CR1;
		if (pclk <= 40'000'000U) {
			return;
		}
		(void)SPI->CR1;
		// pclk <= 50MHz
	}

	inline void disable_spi() {
		SPI->CR1 &= ~SPI_CR1_SPE;
	}

	inline void wait_spi_disabled() {
		while (SPI->SR & SPI_SR_BSY);
	}

	/** Also clears 'RX overrun flag'*/
	inline void empty_spi_rx_fifo() {
		do {
			(void)SPI->DR;
		} while (SPI->SR & SPI_SR_RXNE);
	}

	inline void enable_spi() {
		SPI->CR1 |= SPI_CR1_SPE;
	}
}