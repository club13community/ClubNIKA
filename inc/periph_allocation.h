//
// Created by independent-variable on 3/15/2024.
//

#pragma once
#include "stm32f10x.h"

#define SIM900_UART				USART2
#define SIM900_UART_TX_DMA		DMA1_Channel7
#define SUPPLY_SYSTEM_TIMER		TIM2
#define FINE_TIMER				TIM3
#define TIMING_TIMER			TIM4

#define DAC_TIMER				TIM7
#define DAC_DMA					DMA2_Channel3

#define FLASH_SPI				SPI1
#define FLASH_TX_DMA_CHANNEL	DMA1_Channel3
#define FLASH_RX_DMA_CHANNEL	DMA1_Channel2

#define FINE_TIMER_IRQ_PRIORITY		13U
#define COARSE_TIMER_IRQ_PRIORITY	13U
// same priority for UART and DMA(channel for TX) handlers
#define SIM900_UART_IRQ_PRIORITY	3U
// same for DMA and timer
#define DAC_IRQ_PRIORITY	4U
#define FLASH_IRQ_PRIORITY	1U
// for EXTI
#define KEYBOARD_IRQ_PRIORITY	13U


#define SIM900_UART_TX_DMA_PRIORITY	DMA_Priority_Low
#define DAC_DMA_PRIORITY			DMA_Priority_Medium
#define FLASH_TX_DMA_PRIORITY		DMA_Priority_Medium
#define FLASH_RX_DMA_PRIORITY		DMA_Priority_High

#define TASK_NORMAL_PRIORITY		1U
// detects what was pressed
#define KEYBOARD_SERVICE_PRIORITY	2U