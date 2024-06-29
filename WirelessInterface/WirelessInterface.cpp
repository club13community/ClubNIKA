/*
 * WirelessInterface.c
 *
 *  Created on: 1 трав. 2020 р.
 *      Author: MaxCm
 */
#include "WirelessInterface.h"
#include "stm32f10x.h"

#define CS_PORT	GPIOB
#define CS_PIN	GPIO_Pin_12

#define CE_PORT	GPIOD
#define CE_PIN	GPIO_Pin_8

#define MOSI_PORT 	GPIOB
#define MOSI_PIN	GPIO_Pin_15

#define MISO_PORT 	GPIOB
#define MISO_PIN	GPIO_Pin_14

#define SCK_PORT	GPIOB
#define SCK_PIN		GPIO_Pin_13

#define INT_PORT	GPIOD
#define INT_PIN		GPIO_Pin_9

#define SPI	SPI2