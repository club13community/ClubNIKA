//
// Created by independent-variable on 3/15/2024.
//

#pragma once
#include "stm32f10x.h"

#define SIM900_UART				USART2
#define SIM900_UART_TX_DMA		DMA1_Channel7
#define SUPPLY_SYSTEM_TIMER		TIM2
#define USER_INTERFACE_TIMER	TIM3
#define TIMING_TIMER			TIM4

// same priority for UART and DMA(channel for TX) handlers
#define SIM900_UART_IRQ_PRIORITY	3U

#define SIM900_UART_TX_DMA_PRIORITY	DMA_Priority_Low
