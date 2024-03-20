//
// Created by independent-variable on 3/15/2024.
//
#include "tim_utils.h"
#include "rcc_utils.h"

uint32_t utils::get_int_clock_frequency(TIM_TypeDef * tim) {
	RCC_ClocksTypeDef clk_conf;
	RCC_GetClocksFreq(&clk_conf);
	uint32_t bus_clk = is_APB1_periph(tim) ? clk_conf.PCLK1_Frequency : clk_conf.PCLK2_Frequency;
	return bus_clk == clk_conf.HCLK_Frequency ? bus_clk : bus_clk<<1;
}