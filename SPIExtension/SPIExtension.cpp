/*
 * SPIExtention.c
 *
 *  Created on: 1 трав. 2020 р.
 *      Author: MaxCm
 */

#include "SPIExtension.h"
#include "stm32f10x.h"

#define CS_PORT		GPIOD
#define CS_PIN 		GPIO_Pin_7

#define MOSI_PORT	GPIOB
#define MOSI_PIN	GPIO_Pin_5

#define SCK_PORT	GPIOB
#define SCK_PIN		GPIO_Pin_3

#define MISO_PORT	GPIOB
#define MISO_PIN	GPIO_Pin_4

#define SPI			SPI3