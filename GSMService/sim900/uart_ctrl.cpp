//
// Created by independent-variable on 3/20/2024.
//
#include <stdint.h>
#include "./uart_ctrl.h"
#include "./execution.h"
#include "stm32f10x_usart.h"
#include "rcc_utils.h"
#include "nvic_utils.h"
#include "sim900_isr.h"
#include "./uart_periph.h"

#if configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY >= SIM900_DMA_IRQ_PRIORITY
#error FreeRTOS API may be used in 'UART sent ISR'
#endif

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
	// Interrupt for receiving
	nvicInitStruct.NVIC_IRQChannel = get_IRQn(UART);
	nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
	nvicInitStruct.NVIC_IRQChannelPreemptionPriority = UART_IRQ_PRIORITY;
	nvicInitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvicInitStruct);

	// Interrupt for end of transmitting
	nvicInitStruct.NVIC_IRQChannel = get_IRQn(DMA_CHANNEL);
	nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
	nvicInitStruct.NVIC_IRQChannelPreemptionPriority = DMA_IRQ_PRIORITY;
	nvicInitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvicInitStruct);
}

static BaseType_t (* volatile on_sent)();

/** Does not check if module is turned on, does not check concurrent access.
 * @param command '\r'-ended command, should not change before send is completed
 * @param length takes into account tailing '\r'
  * */
void sim900::send(const char * command, uint16_t length) {
	on_sent = nullptr;
	send_via_dma(command, length);
}

void sim900::send(const char * command, uint16_t length, BaseType_t (* callback)()) {
	on_sent = callback;
	send_via_dma(command, length);
}

bool sim900::is_sent() {
	return is_sent_via_dma();
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
	on_sent_via_dma();
	portYIELD_FROM_ISR(on_sent != nullptr ? on_sent() : pdFALSE);
}