//
// Created by independent-variable on 3/15/2024.
//

#pragma once
#include "stm32f10x.h"

#define SIM900_UART				USART2
#define SUPPLY_SYSTEM_TIMER		TIM2
#define USER_INTERFACE_TIMER	TIM3
#define TIMING_TIMER			TIM4

#define SIM900_UART_IRQ_PRIORITY	3U
