//
// Created by independent-variable on 3/21/2024.
//
#pragma once
#include <exception>
#include "stm32f10x.h"

inline DMA_TypeDef * get_DMA(DMA_Channel_TypeDef * ch) {
	if (ch == DMA1_Channel1 || ch == DMA1_Channel2 || ch == DMA1_Channel3 || ch == DMA1_Channel4 || ch == DMA1_Channel5
		|| ch == DMA1_Channel6 || ch == DMA1_Channel7) {
		return DMA1;
	}
	if (ch == DMA2_Channel1 || ch == DMA2_Channel2 || ch == DMA2_Channel3 || ch == DMA2_Channel4
		|| ch == DMA2_Channel5) {
		return DMA2;
	}
	throw std::exception();
}

inline uint8_t get_channel_index(DMA_Channel_TypeDef * ch) {
	if (ch == DMA1_Channel1 || ch == DMA2_Channel1) {
		return 0;
	}
	if (ch == DMA1_Channel2 || ch == DMA2_Channel2) {
		return 1;
	}
	if (ch == DMA1_Channel3 || ch == DMA2_Channel3) {
		return 2;
	}
	if (ch == DMA1_Channel4 || ch == DMA2_Channel4) {
		return 3;
	}
	if (ch == DMA1_Channel5 || ch == DMA2_Channel5) {
		return 4;
	}
	if (ch == DMA1_Channel6) {
		return 5;
	}
	if (ch == DMA1_Channel7) {
		return 6;
	}
	throw std::exception();
}

inline DMA_Channel_TypeDef * get_DMA_Channel_for_TX(USART_TypeDef * uart) {
	switch ((uint32_t)uart) {
		case USART1_BASE: return DMA1_Channel4;
		case USART2_BASE: return DMA1_Channel7;
		case USART3_BASE: return DMA1_Channel2;
		case UART4_BASE: return DMA2_Channel5;
		default: throw std::exception();
	}
}

inline DMA_Channel_TypeDef * get_DMA_Channel_for_RX(USART_TypeDef * uart) {
	switch ((uint32_t)uart) {
		case USART1_BASE: return DMA1_Channel5;
		case USART2_BASE: return DMA1_Channel6;
		case USART3_BASE: return DMA1_Channel3;
		case UART4_BASE: return DMA2_Channel2;
		default: throw std::exception();
	}
}
