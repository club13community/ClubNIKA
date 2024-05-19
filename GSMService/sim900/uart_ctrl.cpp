//
// Created by independent-variable on 3/20/2024.
//
#include <stdint.h>
#include "sim900_isr.h"
#include "./uart_ctrl.h"
#include "./uart_callbacks.h"
#include "./uart_periph.h"

#if configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY >= SIM900_DMA_IRQ_PRIORITY
#error FreeRTOS API may be used in 'UART sent' ISR
#endif

#if configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY >= SIM900_EXTI_IRQ_PRIORITY
#error FreeRTOS API may be used in EXTI ISR(triggered by 'UART sent' ISR)
#endif

#if configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY <= SIM900_UART_IRQ_PRIORITY
#error 'UART received' ISR should not be masked by FreeRTOS API
#endif

namespace sim900 {
	rx_buffer_t rx_buffer;
}

void sim900::init_uart_ctrl() {
	init_uart();
	init_dma();
	init_exti();
}

static BaseType_t (* volatile on_sent)();

/** Does not check if module is turned on, does not check concurrent access.
 * @param command should not change before send is completed
 * @param length takes into account tailing '\r'(or other terminator)
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
		 trigger_exti();
	}
}

void sim900::handle_dma_interrupt() {
	on_sent_via_dma();
	portYIELD_FROM_ISR(on_sent != nullptr ? on_sent() : pdFALSE);
}

void sim900::handle_exti_interrupt() {
	clear_exti_flag();
	portYIELD_FROM_ISR(on_received());
}