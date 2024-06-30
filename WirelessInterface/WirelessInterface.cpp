/*
 * WirelessInterface.c
 *
 *  Created on: 1 трав. 2020 р.
 *      Author: MaxCm
 */
#include "WirelessInterface.h"
#include "stm32f10x.h"
#include "rcc_utils.h"

#define CS_PORT	GPIOB
#define CS_PIN	GPIO_Pin_12

#define MOSI_PORT 	GPIOB
#define MOSI_PIN	GPIO_Pin_15

#define MISO_PORT 	GPIOB
#define MISO_PIN	GPIO_Pin_14

#define SCK_PORT	GPIOB
#define SCK_PIN		GPIO_Pin_13

#define CE_PORT	GPIOD
#define CE_PIN	GPIO_Pin_8

#define INT_PORT	GPIOD
#define INT_PIN		GPIO_Pin_9

#define SPI	SPI2

void wireless::init_periph() {
	GPIO_InitTypeDef io_conf;
	io_conf.GPIO_Mode = GPIO_Mode_IPU;
	io_conf.GPIO_Speed = GPIO_Speed_2MHz;

	enable_periph_clock(GPIOB);
	io_conf.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_15 | GPIO_Pin_14 | GPIO_Pin_13;
	GPIO_Init(GPIOB, &io_conf);

	enable_periph_clock(GPIOD);
	io_conf.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_Init(GPIOD, &io_conf);
}