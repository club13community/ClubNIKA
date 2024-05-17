//
// Created by independent-variable on 5/17/2024.
//

#pragma once
#include "periph_allocation.h"

#define UART				SIM900_UART
#define UART_IRQ_PRIORITY	SIM900_UART_IRQ_PRIORITY
#define TX_PORT			GPIOD
#define TX_PIN			GPIO_Pin_5
#define RX_PORT			GPIOD
#define RX_PIN			GPIO_Pin_6

#define DMA_CHANNEL		SIM900_UART_TX_DMA
#define DMA_PRIORITY	SIM900_UART_TX_DMA_PRIORITY
#define DMA_IRQ_PRIORITY	SIM900_DMA_IRQ_PRIORITY

#define DMA				DMA1
#define DMA_ISR_TCIFx	DMA_ISR_TCIF7
#define DMA_IFCR_CTCIFx	DMA_IFCR_CTCIF7

namespace sim900 {
	inline void activate_uart() {
		GPIO_InitTypeDef io_conf = {.GPIO_Pin = TX_PIN, .GPIO_Speed = GPIO_Speed_2MHz, .GPIO_Mode = GPIO_Mode_AF_PP};
		GPIO_Init(TX_PORT, &io_conf);
		USART_Cmd(UART, ENABLE);
	}

	inline void suspend_uart() {
		USART_Cmd(UART, DISABLE);
		GPIO_InitTypeDef io_conf = {.GPIO_Pin = TX_PIN, .GPIO_Mode = GPIO_Mode_IN_FLOATING};
		GPIO_Init(TX_PORT, &io_conf);
	}

	inline void send_via_dma(const char * data, uint16_t length) {
		DMA->IFCR = DMA_IFCR_CTCIFx;
		DMA_CHANNEL->CMAR = (uint32_t)data;
		DMA_CHANNEL->CNDTR = length;
		DMA_CHANNEL->CCR |= DMA_CCR1_EN | DMA_CCR1_TCIE;
	}

	/** Disables DMA and TC interrupt, leaving TC flag set*/
	inline void on_sent_via_dma() {
		DMA_CHANNEL->CCR &= ~DMA_CCR1_EN & ~DMA_CCR1_TCIE;
	}

	inline bool is_sent_via_dma() {
		return DMA->ISR & DMA_ISR_TCIFx;
	}
}