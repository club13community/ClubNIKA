//
// Created by independent-variable on 6/28/2024.
//

#pragma once
#include <stdint.h>
#include "stm32f10x.h"

/** Approximate start-up time of HSE. */
#define HSE_START_us	1000U
/** Approximate start-up time of PLL. */
#define PLL_START_us	1000U

#define SYSCLK_FREQ		48'000'000U
#define AHB_FREQ		24'000'000U
#define APB1_FREQ		12'000'000U
#define APB2_FREQ		12'000'000U
#define ADC_FREQ		2'000'000U
