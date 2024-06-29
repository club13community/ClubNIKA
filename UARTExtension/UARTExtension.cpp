/*
 * UARTExtension.h
 *
 *  Created on: 1 трав. 2020 р.
 *      Author: MaxCm
 */


#include "UARTExtension.h"
#include "stm32f10x.h"

#define TX_PORT GPIOB
#define TX_PIN	GPIO_Pin_10

#define RX_PORT GPIOB
#define RX_PIN	GPIO_Pin_11

// general purpose pin
#define AUX_PORT	GPIOE
#define AUX_PIN		GPIO_Pin_13

#define UART	USART3
