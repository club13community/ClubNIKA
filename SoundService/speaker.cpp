//
// Created by independent-variable on 3/23/2024.
//
#include "./speaker.h"
#include "./speaker_config.h"
#include "periph_allocation.h"
#include "stm32f10x.h"
#include "rcc_utils.h"
#include "tim_utils.h"
#include "dma_utils.h"
#include "dac_utils.h"
#include "nvic_utils.h"
#include <exception>

void speaker::initPeripherals() {
	GPIO_InitTypeDef io_conf = {0};
	io_conf.GPIO_Mode = GPIO_Mode_Out_PP;
	io_conf.GPIO_Speed = GPIO_Speed_2MHz;

	enable_periph_clock(MUTE_PORT);
	// todo: change schematic to allow 'initial mute'
	unmute(); // pull-down corresponding pin - amp. is connected to 4V which may not settle yet
	io_conf.GPIO_Pin = MUTE_PIN;
	GPIO_Init(MUTE_PORT, &io_conf);

	enable_periph_clock(INPUT_SELECT_PORT);
	selectDac();
	io_conf.GPIO_Pin = INPUT_SELECT_PIN;
	GPIO_Init(INPUT_SELECT_PORT, &io_conf);

	// Timer to trigger DAC
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

	// DMA to serve DAC
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

	// DAC
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
	DAC_InitTypeDef dac_conf = {0};
	dac_conf.DAC_Trigger = get_trigger_selection(DAC_CHANNEL, DAC_TIMER);
	dac_conf.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
	dac_conf.DAC_WaveGeneration = DAC_WaveGeneration_None;
	DAC_Init(DAC_CHANNEL, &dac_conf);
	DAC_DMACmd(DAC_CHANNEL, ENABLE);
	// write initial 0
	DAC->DHR8R1 = 0;
	DAC->SWTRIGR = DAC_SWTRIGR_SWTRIG1;
	// will enable later

	// DAC pin
	enable_periph_clock(DAC_PORT);
	io_conf.GPIO_Pin = DAC_PIN;
	io_conf.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(DAC_PORT, &io_conf);

	NVIC_InitTypeDef nvicConf = {0};
	nvicConf.NVIC_IRQChannelPreemptionPriority = DAC_IRQ_PRIORITY;
	nvicConf.NVIC_IRQChannelCmd = ENABLE;
	// NVIC for timer
	nvicConf.NVIC_IRQChannel = get_IRQn(DAC_TIMER);
	NVIC_Init(&nvicConf);
	// NVIC for DMA
	nvicConf.NVIC_IRQChannel = get_IRQn(DAC_DMA);
	NVIC_Init(&nvicConf);
}

using namespace speaker;

static volatile DataSupplier nextData;
static volatile Callback end;

void speaker::mute() {
	GPIO_SetBits(MUTE_PORT, MUTE_PIN);
}

void speaker::unmute() {
	GPIO_ResetBits(MUTE_PORT, MUTE_PIN);
}

void speaker::selectDac() {
	GPIO_ResetBits(INPUT_SELECT_PORT, INPUT_SELECT_PIN);
}

void speaker::selectSim900() {
	GPIO_SetBits(INPUT_SELECT_PORT, INPUT_SELECT_PIN);
}

void speaker::playOnDac(uint16_t samplePeriod_us, DataSupplier getData, Callback onEnd) {
	Data data = getData();
	nextData = getData;
	end = onEnd;

	DAC_TIMER->ARR = samplePeriod_us;
	DAC_TIMER->CNT = 0;
	DAC_DMA->CMAR = (uint32_t)data.samples;
	DAC_DMA->CNDTR = data.samplesLength;

	DAC->CR |= DAC_CR_EN1;
	DAC_DMA->CCR |= DMA_CCR1_EN; // all CCRx are equal
	DAC_TIMER->CR1 |= TIM_CR1_CEN;
}

void speaker::dmaISR() {
	Data data = nextData();
	if (data.isEnd()) {
		// last sample is still not played
		DAC_DMA->CCR &= ~DMA_CCR1_EN;
		DAC_TIMER->SR = ~TIM_SR_UIF;
		DAC_TIMER->DIER = TIM_DIER_UIE;
	} else {
		DAC_DMA->CCR &= ~DMA_CCR1_EN;
		DAC_DMA->CMAR = (uint32_t)data.samples;
		DAC_DMA->CNDTR = data.samplesLength;
		DAC_DMA->CCR |= DMA_CCR1_EN;
	}
	DMA2->IFCR = DMA_IFCR_CTCIF3;
}

void speaker::timerISR() {
	DAC_TIMER->DIER = 0U;
	DAC_TIMER->CR1 &= ~TIM_CR1_CEN;
	DAC->CR &= ~DAC_CR_EN1;
	end();
}
