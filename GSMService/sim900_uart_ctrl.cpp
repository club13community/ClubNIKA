//
// Created by independent-variable on 3/20/2024.
//
#include <stdint.h>
#include "config.h"
#include "sim900_uart_ctrl.h"
#include "stm32f10x_usart.h"
#include "rcc_utils.h"
#include "nvic_utils.h"
#include "sim900_power_ctrl.h"

#define CHAR_0_PRESENT	0x01U
#define CHAR_1_PRESENT	0x02U
#define FIFO_EMPTY		0x00U

static class {
private:
	unsigned char fifo[2];
	uint8_t fifo_level;
	char message[MAX_UART_MESSAGE_LENGTH];
	uint16_t index = 0;
	bool ok = true, end = true;

public:
	void add(char symbol) {
		if (end) {
			fifo_level = FIFO_EMPTY;
			index = 0;
			ok = true;
			// end is set later
		}
		if (index < MAX_UART_MESSAGE_LENGTH) {
			message[index] = fifo[0];
			index += fifo_level & CHAR_0_PRESENT; // increment only if fifo[0] contains something
		} else {
			ok = false;
		}
		fifo[0] = fifo[1];
		fifo[1] = symbol;
		fifo_level = (fifo_level >> 1) | CHAR_1_PRESENT;
		end = fifo_level == (CHAR_1_PRESENT | CHAR_0_PRESENT)
				&& *(uint16_t *)fifo == 0x0A0D; // last 2 chars are CR(0x0D), LF(0x0A)
	}

	void mark_message_corrupted() {
		ok = false;
	}

	uint16_t get_message_length() {
		return ok && end ? index : 0;
	}

	char * get_message() {
		return message;
	}

} rx_stream;

void sim900::init_uart_ctrl() {
	GPIO_InitTypeDef pinInitStruct;
	NVIC_InitTypeDef nvicInitStruct;
	DMA_InitTypeDef dmaInitStruct;

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

	// UART
	enable_periph_clock(UART);
	GPIO_PinRemapConfig(GPIO_Remap_USART2, ENABLE); // todo: impl. AFIO utils
	USART_InitTypeDef uartInitStruct;
	uartInitStruct.USART_BaudRate=57600;
	uartInitStruct.USART_HardwareFlowControl=USART_HardwareFlowControl_None;
	uartInitStruct.USART_Mode=USART_Mode_Rx | USART_Mode_Tx;
	uartInitStruct.USART_Parity=USART_Parity_No;
	uartInitStruct.USART_StopBits=USART_StopBits_1;
	uartInitStruct.USART_WordLength=USART_WordLength_8b;
	USART_Init(UART, &uartInitStruct);
	USART_ITConfig(UART, USART_IT_RXNE, ENABLE);

	/*dmaInitStruct.DMA_Mode=DMA_Mode_Normal;
	dmaInitStruct.DMA_DIR=DMA_DIR_PeripheralDST;
	dmaInitStruct.DMA_M2M=DMA_M2M_Disable;
	dmaInitStruct.DMA_Priority=DMA_Priority_High;

	dmaInitStruct.DMA_MemoryBaseAddr=(uint32_t)tx_buf;
	dmaInitStruct.DMA_MemoryDataSize=DMA_MemoryDataSize_Byte;
	dmaInitStruct.DMA_MemoryInc=DMA_MemoryInc_Enable;

	dmaInitStruct.DMA_PeripheralBaseAddr=(uint32_t)&(USART3->DR);
	dmaInitStruct.DMA_PeripheralDataSize=DMA_PeripheralDataSize_Byte;
	dmaInitStruct.DMA_PeripheralInc=DMA_PeripheralInc_Disable;

	dmaInitStruct.DMA_BufferSize=4;

	DMA_Init(DMA1_Channel2, &dmaInitStruct);
	DMA_ITConfig(DMA1_Channel2, DMA_IT_TC, ENABLE);

	USART_DMACmd(USART3, USART_DMAReq_Tx, ENABLE);*/

	IRQn_Type irqn = get_IRQn(UART);
	nvicInitStruct.NVIC_IRQChannel = irqn;
	nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
	nvicInitStruct.NVIC_IRQChannelPreemptionPriority = UART_IRQ_PRIORITY;
	nvicInitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvicInitStruct);

	/*nvicInitStruct.NVIC_IRQChannel = DMA1_Channel2_IRQn;
	nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
	nvicInitStruct.NVIC_IRQChannelPreemptionPriority = EXTERNAL_UART_TX_PRIORITY;
	nvicInitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvicInitStruct);*/
}

void sim900::handle_uart_interrupt() {
	bool symbol_corrupted = UART->SR & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE);
	uint8_t symbol = UART->DR;
	if (symbol_corrupted) {
		rx_stream.mark_message_corrupted();
	} else {
		rx_stream.add(symbol);
	}
	uint16_t length = rx_stream.get_message_length();
	if (length > 0) {
		char * message = rx_stream.get_message();
		bool handled = sim900::handle_power_ctrl_message(message, length);
	}
}