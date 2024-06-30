//
// Created by independent-variable on 6/30/2024.
//

#pragma once
#include "stm32f10x.h"

#define DP_PORT	GPIOA
#define DP_PIN	GPIO_Pin_12

#define DM_PORT	GPIOA
#define DM_PIN	GPIO_Pin_11

// to detect VBUS
#define VBUS_PORT	GPIOA
#define VBUS_PIN	GPIO_Pin_10