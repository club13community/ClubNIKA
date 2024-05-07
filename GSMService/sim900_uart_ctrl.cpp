//
// Created by independent-variable on 3/20/2024.
//
#include <stdint.h>
#include "./config.h"
#include "sim900_driver.h"
#include "./sim900_uart_ctrl.h"
#include "stm32f10x_usart.h"
#include "rcc_utils.h"
#include "nvic_utils.h"
#include "dma_utils.h"
#include "./sim900_power_ctrl.h"

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

static DMA_TypeDef * dma;
static uint32_t dma_ifcr_ctcif;
static void (* volatile tx_callback)();

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
	GPIO_PinRemapConfig(GPIO_Remap_USART2, ENABLE); // todo: impl. AFIO utils
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
 * @param callback is invoked after transfer is completed
 * */
void sim900::send(char * command, uint16_t length, void (* callback)()) {
	DMA_CHANNEL->CMAR = (uint32_t)command;
	DMA_CHANNEL->CNDTR = length;
	tx_callback = callback;
	DMA_CHANNEL->CCR |= DMA_CCR1_EN;
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

static volatile uint32_t ccr;
void sim900::handle_dma_interrupt() {
	DMA_CHANNEL->CCR &= ~DMA_CCR1_EN;
	dma->IFCR = dma_ifcr_ctcif;
	tx_callback();
}