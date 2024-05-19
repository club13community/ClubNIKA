//
// Created by independent-variable on 5/17/2024.
//

#pragma once
#include "periph_allocation.h"
#include "stm32f10x.h"
#include "rcc_utils.h"
#include "nvic_utils.h"

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

#define EXTI_IMR_MRx		SIM900_EXTI_LINE
#define EXTI_SWIER_SWIERx	SIM900_EXTI_LINE
#define EXTI_PR_PRx			SIM900_EXTI_LINE

namespace sim900 {
	inline void init_uart() {
		GPIO_InitTypeDef pinInitStruct;
		// Tx pin
		enable_periph_clock(TX_PORT);
		pinInitStruct.GPIO_Pin=TX_PIN;
		pinInitStruct.GPIO_Mode=GPIO_Mode_IN_FLOATING; // SIM900 is power-downed
		pinInitStruct.GPIO_Speed=GPIO_Speed_2MHz;
		GPIO_Init(TX_PORT, &pinInitStruct);

		// Rx pin
		enable_periph_clock(RX_PORT);
		pinInitStruct.GPIO_Pin=RX_PIN;
		pinInitStruct.GPIO_Mode=GPIO_Mode_IN_FLOATING; // SIM900 is power-downed
		pinInitStruct.GPIO_Speed=GPIO_Speed_2MHz;
		GPIO_Init(RX_PORT, &pinInitStruct);

		USART_InitTypeDef uartInitStruct;
		// UART
		enable_periph_clock(UART);
		GPIO_PinRemapConfig(GPIO_Remap_USART2, ENABLE);
		uartInitStruct.USART_BaudRate=57600;
		uartInitStruct.USART_HardwareFlowControl=USART_HardwareFlowControl_None;
		uartInitStruct.USART_Mode=USART_Mode_Rx | USART_Mode_Tx;
		uartInitStruct.USART_Parity=USART_Parity_No;
		uartInitStruct.USART_StopBits=USART_StopBits_1;
		uartInitStruct.USART_WordLength=USART_WordLength_8b;
		USART_Init(UART, &uartInitStruct);
		USART_ITConfig(UART, USART_IT_RXNE, ENABLE);
		USART_DMACmd(UART, USART_DMAReq_Tx, ENABLE);

		NVIC_InitTypeDef nvicInitStruct;
		// Interrupt for receiving
		nvicInitStruct.NVIC_IRQChannel = get_IRQn(UART);
		nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
		nvicInitStruct.NVIC_IRQChannelPreemptionPriority = UART_IRQ_PRIORITY;
		nvicInitStruct.NVIC_IRQChannelSubPriority = 0;
		NVIC_Init(&nvicInitStruct);
	}

	inline void init_dma() {
		// DMA for transmitting
		enable_periph_clock(DMA);
		DMA_InitTypeDef dmaInitStruct = {0};
		dmaInitStruct.DMA_Mode=DMA_Mode_Normal;
		dmaInitStruct.DMA_DIR=DMA_DIR_PeripheralDST;
		dmaInitStruct.DMA_M2M=DMA_M2M_Disable;
		dmaInitStruct.DMA_Priority=DMA_PRIORITY;

		dmaInitStruct.DMA_MemoryDataSize=DMA_MemoryDataSize_Byte;
		dmaInitStruct.DMA_MemoryInc=DMA_MemoryInc_Enable;

		dmaInitStruct.DMA_PeripheralBaseAddr=(uint32_t)&UART->DR;
		dmaInitStruct.DMA_PeripheralDataSize=DMA_PeripheralDataSize_Byte;
		dmaInitStruct.DMA_PeripheralInc=DMA_PeripheralInc_Disable;

		DMA_Init(DMA_CHANNEL, &dmaInitStruct);
		// interrupts are enabled right before transferring

		NVIC_InitTypeDef nvicInitStruct;
		// Interrupt for end of transmitting
		nvicInitStruct.NVIC_IRQChannel = get_IRQn(DMA_CHANNEL);
		nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
		nvicInitStruct.NVIC_IRQChannelPreemptionPriority = DMA_IRQ_PRIORITY;
		nvicInitStruct.NVIC_IRQChannelSubPriority = 0;
		NVIC_Init(&nvicInitStruct);
	}

	inline void init_exti() {
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
		EXTI->IMR |= EXTI_IMR_MRx;

		NVIC_InitTypeDef nvic_conf;
		nvic_conf.NVIC_IRQChannel = EXTI0_IRQn;
		nvic_conf.NVIC_IRQChannelPreemptionPriority = SIM900_EXTI_IRQ_PRIORITY;
		nvic_conf.NVIC_IRQChannelSubPriority = 0;
		nvic_conf.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&nvic_conf);
	}

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

	inline void trigger_exti() {
		EXTI->SWIER = EXTI_SWIER_SWIERx;
	}

	inline void clear_exti_flag() {
		EXTI->PR = EXTI_PR_PRx;
	}
}