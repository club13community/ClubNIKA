//
// Created by independent-variable on 3/16/2024.
//

#pragma once
#include <exception>
#include "stm32f10x.h"

inline void enable_periph_clock(TIM_TypeDef * tim) {
	uint32_t enr = 0;

	switch ((uint32_t)tim) {
		case TIM7_BASE: enr = RCC_APB1ENR_TIM7EN; break;
		case TIM6_BASE: enr = RCC_APB1ENR_TIM6EN; break;
		case TIM5_BASE: enr = RCC_APB1ENR_TIM5EN; break;
		case TIM4_BASE: enr = RCC_APB1ENR_TIM4EN; break;
		case TIM3_BASE: enr = RCC_APB1ENR_TIM3EN; break;
		case TIM2_BASE: enr = RCC_APB1ENR_TIM2EN; break;
	}
	if (enr) {
		RCC->APB1ENR |= enr;
		enr = RCC->APB1ENR;
		return;
	}

	switch ((uint32_t)tim) {
		case TIM8_BASE: enr = RCC_APB2ENR_TIM8EN; break;
		case TIM1_BASE: enr = RCC_APB2ENR_TIM1EN; break;
	}
	if (enr) {
		RCC->APB2ENR |= enr;
		enr = RCC->APB2ENR;
		return;
	}

	throw std::exception();
}

inline void enable_periph_clock(USART_TypeDef * usart) {
	if (usart == USART1) {
		RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
		(void)RCC->APB2ENR;
		return;
	}
	switch ((uint32_t)usart) {
		case USART2_BASE: RCC->APB1ENR |= RCC_APB1ENR_USART2EN; break;
		case USART3_BASE: RCC->APB1ENR |= RCC_APB1ENR_USART3EN; break;
		case UART4_BASE: RCC->APB1ENR |= RCC_APB1ENR_UART4EN; break;
		case UART5_BASE: RCC->APB1ENR |= RCC_APB1ENR_UART5EN; break;
		default: throw std::exception();
	}
	(void)RCC->APB1ENR;
}

inline void enable_periph_clock(GPIO_TypeDef * gpio) {
	switch ((uint32_t)gpio) {
		case GPIOA_BASE: RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; break;
		case GPIOB_BASE: RCC->APB2ENR |= RCC_APB2ENR_IOPBEN; break;
		case GPIOC_BASE: RCC->APB2ENR |= RCC_APB2ENR_IOPCEN; break;
		case GPIOD_BASE: RCC->APB2ENR |= RCC_APB2ENR_IOPDEN; break;
		case GPIOE_BASE: RCC->APB2ENR |= RCC_APB2ENR_IOPEEN; break;
		case GPIOF_BASE: RCC->APB2ENR |= RCC_APB2ENR_IOPFEN; break;
		case GPIOG_BASE: RCC->APB2ENR |= RCC_APB2ENR_IOPGEN; break;
		default: throw std::exception();
	}
	(void)RCC->APB2ENR;
}

inline void enable_periph_clock(DMA_TypeDef * dma) {
	uint32_t enr;
	if (dma == DMA1) {
		enr = RCC_AHBENR_DMA1EN;
	} else if (dma == DMA2) {
		enr = RCC_AHBENR_DMA2EN;
	} else {
		throw std::exception();
	}
	RCC->AHBENR |= enr;
	(void)RCC->AHBENR;
}