//
// Created by independent-variable on 5/28/2024.
//

#pragma once
#include "stm32f10x.h"
#include "periph_allocation.h"
#include "rcc_utils.h"
#include "tim_utils.h"
#include "dma_utils.h"
#include "dac_utils.h"
#include "nvic_utils.h"

#define DAC_CHANNEL	DAC_Channel_1
#define DAC_PORT	GPIOA
#define DAC_PIN		GPIO_Pin_4

#define DMA_IFCR_CTCIFx		DMA_IFCR_CTCIF3

namespace speaker {

	/** Timer triggers DAC */
	void init_timer() {
		enable_periph_clock(DAC_TIMER);
		uint32_t int_clk = get_int_clock_frequency(DAC_TIMER);
		uint32_t ratio = int_clk / 1000000;
		if (ratio == 0) {
			throw std::exception();
		}
		TIM_TimeBaseInitTypeDef tim_conf = {0};
		tim_conf.TIM_CounterMode = TIM_CounterMode_Up;
		tim_conf.TIM_Prescaler = ratio - 1;
		// period will be set later
		TIM_TimeBaseInit(DAC_TIMER, &tim_conf);
		TIM_SelectOutputTrigger(DAC_TIMER, TIM_TRGOSource_Update);
		// will enable later
	}

	/** DMA to serve DAC */
	void init_dma() {
		DMA_TypeDef * dma = get_DMA(DAC_DMA);
		enable_periph_clock(dma);
		DMA_InitTypeDef dma_conf = {0};
		dma_conf.DMA_Mode = DMA_Mode_Normal; // Software emulates double buffer
		dma_conf.DMA_Priority = DAC_DMA_PRIORITY;
		dma_conf.DMA_DIR = DMA_DIR_PeripheralDST;
		dma_conf.DMA_PeripheralBaseAddr = (uint32_t)&DAC->DHR8R1;
		dma_conf.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
		dma_conf.DMA_PeripheralInc = DMA_PeripheralInc_Disable;

		dma_conf.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
		dma_conf.DMA_MemoryInc = DMA_MemoryInc_Enable;
		// buffer address and number of transfers are set later
		DMA_Init(DAC_DMA, &dma_conf);
		DMA_ITConfig(DAC_DMA, DMA_IT_TC, ENABLE);
		// will enable later

		// NVIC for DMA
		NVIC_InitTypeDef nvicConf = {0};
		nvicConf.NVIC_IRQChannelPreemptionPriority = DAC_IRQ_PRIORITY;
		nvicConf.NVIC_IRQChannelCmd = ENABLE;
		nvicConf.NVIC_IRQChannel = get_IRQn(DAC_DMA);
		NVIC_Init(&nvicConf);
	}

	void init_dac() {
		GPIO_InitTypeDef io_conf = {0};
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
		DAC_InitTypeDef dac_conf = {0};
		dac_conf.DAC_Trigger = get_trigger_selection(DAC_CHANNEL, DAC_TIMER);
		dac_conf.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
		dac_conf.DAC_WaveGeneration = DAC_WaveGeneration_None;
		DAC_Init(DAC_CHANNEL, &dac_conf);
		DAC_DMACmd(DAC_CHANNEL, ENABLE);
		DAC_Cmd(DAC_CHANNEL, ENABLE);
		// write initial 0
		DAC->DHR8R1 = 0;
		DAC->SWTRIGR = DAC_SWTRIGR_SWTRIG1;

		// DAC pin
		enable_periph_clock(DAC_PORT);
		io_conf.GPIO_Pin = DAC_PIN;
		io_conf.GPIO_Mode = GPIO_Mode_AIN;
		GPIO_Init(DAC_PORT, &io_conf);
	}

	inline void set_sample_period(uint16_t period_us) {
		DAC_TIMER->ARR = period_us;
	}

	/** @param sample value which is put to "data register" and "shadow conversion register" */
	inline void put_initial_samples(uint8_t sample) {
		DAC->DHR8R1 = sample;
		DAC->SWTRIGR = DAC_SWTRIGR_SWTRIG1;
		DAC->DHR8R1 = sample;
	}

	inline void start_dma(uint8_t * data, uint16_t size) {
		DAC_DMA->CMAR = (uint32_t)data;
		DAC_DMA->CNDTR = size;
		DAC_DMA->CCR |= DMA_CCR1_EN; // all CCRx are equal
	}

	inline void stop_dma() {
		DAC_DMA->CCR &= ~DMA_CCR1_EN;
		while (DAC_DMA->CCR & DMA_CCR1_EN);
	}

	/** Clears flags and EN bit */
	inline void begin_dma_isr() {
		DAC_DMA->CCR &= ~DMA_CCR1_EN;
		DMA2->IFCR = DMA_IFCR_CTCIFx;
	}

	inline void start_timer() {
		DAC_TIMER->CR1 |= TIM_CR1_CEN;
	}

	inline void stop_timer() {
		DAC_TIMER->CR1 &= ~TIM_CR1_CEN;
		DAC_TIMER->CNT = 0;
	}
}