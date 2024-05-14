//
// Created by independent-variable on 3/20/2024.
//
#include <stdint.h>
#include "./uart_ctrl.h"
#include "./execution.h"
#include "config.h"
#include "stm32f10x_usart.h"
#include "rcc_utils.h"
#include "nvic_utils.h"
#include "dma_utils.h"
#include "sim900_isr.h"

static DMA_TypeDef * dma;
static uint32_t dma_ifcr_ctcif;

void sim900::init_uart_ctrl() {
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

	// DMA for transmitting
	dma = get_DMA(DMA_CHANNEL);
	dma_ifcr_ctcif = DMA_IFCR_CTCIF1 << (get_channel_index(DMA_CHANNEL) * 4);
	enable_periph_clock(dma);
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
	DMA_ITConfig(DMA_CHANNEL, DMA_IT_TC, ENABLE);

	NVIC_InitTypeDef nvicInitStruct;
	// Interrupt for receiving
	nvicInitStruct.NVIC_IRQChannel = get_IRQn(UART);
	nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
	nvicInitStruct.NVIC_IRQChannelPreemptionPriority = UART_IRQ_PRIORITY;
	nvicInitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvicInitStruct);

	// Interrupt for end of transmitting
	nvicInitStruct.NVIC_IRQChannel = get_IRQn(DMA_CHANNEL);
	nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
	nvicInitStruct.NVIC_IRQChannelPreemptionPriority = UART_IRQ_PRIORITY;
	nvicInitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvicInitStruct);
}

/** Does not check if module is turned on, does not check concurrent access.
 * @param command '\r'-ended command, should not change before send is completed
 * @param length takes into account tailing '\r'
  * */
void sim900::send(const char * command, uint16_t length) {
	DMA_CHANNEL->CMAR = (uint32_t)command;
	DMA_CHANNEL->CNDTR = length;
	DMA_CHANNEL->CCR |= DMA_CCR1_EN;
}

void sim900::handle_uart_interrupt() {
	bool symbol_corrupted = UART->SR & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE);
	uint8_t symbol = UART->DR;
	if (symbol_corrupted) {
		rx_buffer.mark_message_corrupted();
	} else if (rx_buffer.add(symbol)) {
		response_received(); // todo invoke via less prio. IRQ
	}
}

void sim900::handle_dma_interrupt() {
	DMA_CHANNEL->CCR &= ~DMA_CCR1_EN;
	dma->IFCR = dma_ifcr_ctcif;
}