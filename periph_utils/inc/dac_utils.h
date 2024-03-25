//
// Created by independent-variable on 3/24/2024.
//

#pragma once
#include <exception>
#include "stm32f10x.h"

inline uint32_t get_trigger_selection(uint32_t dac_channel, TIM_TypeDef * tim) {
	if (dac_channel != DAC_Channel_1 && dac_channel != DAC_Channel_2) {
		throw std::exception();
	}
	switch ((uint32_t)tim) {
		case TIM6_BASE: return DAC_Trigger_T6_TRGO;
		case TIM8_BASE: return DAC_Trigger_T8_TRGO;
		case TIM7_BASE: return DAC_Trigger_T7_TRGO;
		case TIM5_BASE: return DAC_Trigger_T5_TRGO;
		case TIM2_BASE: return DAC_Trigger_T2_TRGO;
		case TIM4_BASE: return DAC_Trigger_T4_TRGO;
		default: throw std::exception();
	}
}