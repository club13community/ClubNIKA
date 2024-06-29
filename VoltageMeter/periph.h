//
// Created by independent-variable on 3/30/2024.
//

#pragma once
#include "stm32f10x.h"
#include "periph_allocation.h"
#include "ClockControl.h"

#define ADC		VMETER_ADC
#define TIMER	VMETER_ADC_TIMER

#define MEASURE_PORT GPIOA
// current sank by protection zone sensor
#define ZONE_SINK_PIN		GPIO_Pin_1
#define ZONE_SINK_CHANNEL	((uint32_t)ADC_Channel_1)
// current sourced by protection zone sensor
#define ZONE_SOURCE_PIN		GPIO_Pin_0
#define ZONE_SOURCE_CHANNEL	((uint32_t)ADC_Channel_0)
// battery voltage
#define BATTERY_PIN			GPIO_Pin_2
#define BATTERY_CHANNEL		((uint32_t)ADC_Channel_2)

// how long channel will be sampled
#define ZONE_SAMPLING_DURATION		((uint32_t)ADC_SampleTime_71Cycles5)
#define BATTERY_SAMPLING_DURATION	((uint32_t)ADC_SampleTime_71Cycles5)

// time between samples in us. Settling time for zone sensor is theoretically 20us
#define SAMPLING_PERIOD		1000U

#define ADC_SMPR2_SMP0_Pos		0U
#define ADC_SMPR2_SMP1_Pos		3U
#define ADC_SMPR2_SMP2_Pos		6U

#define ADC_SQR1_L_Pos			20U
#define ADC_SQR3_SQ1_Pos		0U

#define ADC_JSQR_JSQ4_Pos		15U
#define ADC_JSQR_JL_Pos			20U

#define ADC_JSQR_JSQ4_Msk		((uint32_t)0x1F << ADC_JSQR_JSQ4_Pos)
#define ADC_JSQR_JL_Msk			((uint32_t)0x03 << ADC_JSQR_JL_Pos)

namespace vmeter {
	/** clears flags */
	inline void clear_flags() {
		ADC->SR = ~ADC_SR_EOC & ~ADC_SR_JEOC & ~ADC_SR_STRT & ~ADC_SR_JSTRT;
	}

	inline void init_pins() {
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
		GPIO_InitTypeDef io_conf = {0};
		io_conf.GPIO_Pin = ZONE_SINK_PIN | ZONE_SOURCE_PIN | BATTERY_PIN;
		io_conf.GPIO_Mode = GPIO_Mode_AIN;
		io_conf.GPIO_Speed = GPIO_Speed_2MHz;
		GPIO_Init(MEASURE_PORT, &io_conf);
	}

	inline void init_timer() {
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);

		uint32_t int_clk = clocks::get_freq(TIMER);
		uint32_t prescaler = int_clk / 1000000;
		TIM_TimeBaseInitTypeDef tim_base_conf = {0};
		tim_base_conf.TIM_CounterMode = TIM_CounterMode_Up;
		tim_base_conf.TIM_Prescaler = prescaler - 1; // 1us
		tim_base_conf.TIM_Period = SAMPLING_PERIOD - 1; // actually any value - appropriate value is set later
		tim_base_conf.TIM_RepetitionCounter = 0;
		tim_base_conf.TIM_ClockDivision = TIM_CKD_DIV1;
		TIM_TimeBaseInit(TIMER, &tim_base_conf);

		TIM_SelectOutputTrigger(TIMER, TIM_TRGOSource_Update);
	}

	inline void init_adc() {
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC3, ENABLE);

		ADC_InitTypeDef adc_conf = {0};
		adc_conf.ADC_DataAlign = ADC_DataAlign_Right;
		adc_conf.ADC_Mode = ADC_Mode_Independent;
		adc_conf.ADC_ContinuousConvMode = DISABLE;
		adc_conf.ADC_ScanConvMode = DISABLE;
		adc_conf.ADC_NbrOfChannel = 1;
		adc_conf.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T8_TRGO;
		ADC_Init(ADC, &adc_conf);

		// sampling duration for all channels
		ADC->SMPR2 = BATTERY_SAMPLING_DURATION << ADC_SMPR2_SMP2_Pos | ZONE_SAMPLING_DURATION << ADC_SMPR2_SMP1_Pos
					 | ZONE_SAMPLING_DURATION << ADC_SMPR2_SMP0_Pos;
		// battery voltage as the only channel for regular conversion
		ADC->SQR3 = BATTERY_CHANNEL << ADC_SQR3_SQ1_Pos;
		ADC->SQR1 = (uint32_t)(1 - 1) << ADC_SQR1_L_Pos;
	}

	/** expects, that ADC and timer are not running */
	inline void power_up_adc() {
		// prepare timer to wait 1us
		TIM_SetAutoreload(TIMER, 1);
		TIM_ClearFlag(TIMER, TIM_FLAG_Update);
		ADC_Cmd(ADC, ENABLE);
		TIM_Cmd(TIMER, ENABLE);
		while (TIM_GetFlagStatus(TIMER, TIM_FLAG_Update) == RESET);
		TIM_Cmd(TIMER, DISABLE);
	}

	/** resets prev. calibration before running new; trigger of conversions should be disabled */
	inline void calibrate_adc() {
		ADC_ResetCalibration(ADC);
		while (ADC_GetResetCalibrationStatus(ADC) == SET);
		ADC_StartCalibration(ADC);
		while (ADC_GetCalibrationStatus(ADC) == SET);
	}

	inline void enable_adc_trigger() {
		ADC_ExternalTrigConvCmd(ADC, ENABLE);
	}

	inline void enable_adc_irq() {
		clear_flags();
		ADC_ITConfig(ADC, ADC_IT_EOC, ENABLE);

		NVIC_InitTypeDef nvic_conf;
		nvic_conf.NVIC_IRQChannel = ADC3_IRQn;
		nvic_conf.NVIC_IRQChannelPreemptionPriority = VMETER_ADC_IRQ_PRIORITY;
		nvic_conf.NVIC_IRQChannelSubPriority = 0;
		nvic_conf.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&nvic_conf);
	}

	inline void start_conversions() {
		TIM_SetCounter(TIMER, 0);
		TIM_SetAutoreload(TIMER, SAMPLING_PERIOD - 1);
		TIM_Cmd(TIMER, ENABLE);
	}

	inline void measure_zone_sourcing() {
		// zones are measured as injected
		ADC->JSQR = ADC->JSQR & ~ADC_JSQR_JSQ4_Msk & ~ADC_JSQR_JL_Msk
				| ZONE_SOURCE_CHANNEL << ADC_JSQR_JSQ4_Pos | (uint32_t)(1 - 1) << ADC_JSQR_JL_Pos;
		// auto triggered after regular; interrupt after injected done
		ADC->CR1 = ADC->CR1 & ~ADC_CR1_EOCIE | ADC_CR1_JAUTO | ADC_CR1_JEOCIE;
	}

	inline void measure_zone_sinking() {
		// zones are measured as injected
		ADC->JSQR = ADC->JSQR & ~ADC_JSQR_JSQ4_Msk & ~ADC_JSQR_JL_Msk
					| ZONE_SINK_CHANNEL << ADC_JSQR_JSQ4_Pos | (uint32_t)(1 - 1) << ADC_JSQR_JL_Pos;
		// auto triggered after regular; interrupt after injected done
		ADC->CR1 = ADC->CR1 & ~ADC_CR1_EOCIE | ADC_CR1_JAUTO | ADC_CR1_JEOCIE;
	}

	inline void disable_zone_measurement() {
		ADC->CR1 = ADC->CR1 & ~ADC_CR1_JAUTO & ~ADC_CR1_JEOCIE | ADC_CR1_EOCIE;
	}

	inline uint16_t get_battery_measurement() {
		return ADC->DR;
	}

	inline uint16_t get_zone_measurement() {
		return ADC->JDR1;
	}
}