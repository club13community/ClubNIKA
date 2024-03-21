//
// Created by independent-variable on 3/16/2024.
//

#pragma once
#include <exception>
#include "stm32f10x.h"

inline IRQn_Type get_IRQn(TIM_TypeDef * tim) {
	switch ((uint32_t)tim) {
		case TIM2_BASE: return TIM2_IRQn;
		case TIM3_BASE: return TIM3_IRQn;
		case TIM4_BASE: return TIM4_IRQn;
		case TIM5_BASE: return TIM5_IRQn;
		case TIM6_BASE: return TIM6_IRQn;
		case TIM7_BASE: return TIM7_IRQn;
		default: throw std::exception();
	}
}

inline IRQn_Type get_IRQn(USART_TypeDef * usart) {
	switch ((uint32_t)usart) {
		case USART1_BASE: return USART1_IRQn;
		case USART2_BASE: return USART2_IRQn;
		case USART3_BASE: return USART3_IRQn;
		case UART4_BASE: return UART4_IRQn;
		case UART5_BASE: return UART5_IRQn;
		default: throw std::exception();
	}
}

inline IRQn_Type get_IRQn(DMA_Channel_TypeDef * dma_ch) {
	switch ((uint32_t)dma_ch) {
		case DMA1_Channel1_BASE: return DMA1_Channel1_IRQn;
		case DMA1_Channel2_BASE: return DMA1_Channel2_IRQn;
		case DMA1_Channel3_BASE: return DMA1_Channel3_IRQn;
		case DMA1_Channel4_BASE: return DMA1_Channel4_IRQn;
		case DMA1_Channel5_BASE: return DMA1_Channel5_IRQn;
		case DMA1_Channel6_BASE: return DMA1_Channel6_IRQn;
		case DMA1_Channel7_BASE: return DMA1_Channel7_IRQn;

		case DMA2_Channel1_BASE: return DMA2_Channel1_IRQn;
		case DMA2_Channel2_BASE: return DMA2_Channel2_IRQn;
		case DMA2_Channel3_BASE: return DMA2_Channel3_IRQn;
		case DMA2_Channel4_BASE:
		case DMA2_Channel5_BASE: return DMA2_Channel4_5_IRQn;

		default: throw std::exception();
	}
}