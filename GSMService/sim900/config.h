//
// Created by independent-variable on 3/17/2024.
//

#pragma once
#include "periph_allocation.h"
#include "timing.h"

#define UART				SIM900_UART
#define UART_IRQ_PRIORITY	SIM900_UART_IRQ_PRIORITY
#define TX_PORT			GPIOD
#define TX_PIN			GPIO_Pin_5
#define RX_PORT			GPIOD
#define RX_PIN			GPIO_Pin_6

#define DMA_CHANNEL		SIM900_UART_TX_DMA
#define DMA_PRIORITY	SIM900_UART_TX_DMA_PRIORITY

#define PWR_KEY_PORT	GPIOD
#define PWR_KEY_PIN		GPIO_Pin_4
#define VBAT_SW_PORT	GPIOD
#define VBAT_SW_PIN		GPIO_Pin_1
#define VBAT_DISCHARGE_PORT	GPIOD
#define VBAT_DISCHARGE_PIN	GPIO_Pin_0

#define TIMER (timing::coarse_timer3)

/** time in ms for VBAT settling after connecting to 4V */
#define VBAT_SETTIMG_TIME	300
/** time in ms to discharge VBAT decoupling caps. */
#define VBAT_DISCHARGE_TIME	300

/** Size of buffer for SIM900 commands(and data)*/
#define TX_BUFFER_LENGTH	128
/** Size of circular buffer for SIM900 responses */
#define RX_BUFFER_LENGTH_pow2	9
/** Typical timeout for response from SIM900 */
#define RESP_TIMEOUT_ms 	2000U
/** Timeout for obtaining ID */
#define SEND_SMS_TIMEOUT_ms	10'000U
