/*
 * I2CExtension.c
 *
 *  Created on: 1 трав. 2020 р.
 *      Author: MaxCm
 */
#include "I2CExtension.h"
#include "stm32f10x.h"

#define SDA_PORT	GPIOB
#define SDA_PIN		GPIO_Pin_7

#define SCL_PORT	GPIOB
#define SCL_PIN		GPIO_Pin_6

#define I2C			I2C1