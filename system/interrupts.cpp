/*
 * interrupts.c
 *
 *  Created on: 30 бер. 2020 р.
 *      Author: MaxCm
 */
#include "stm32f10x_exti.h"
#include "stm32f10x_tim.h"
#include "FreeRTOS.h"
#include "SupplySystem.h"
#include "sim900_isr.h"
#include "WirelessInterface.h"
#include "UserInterface.h"
#include "SPIExtension.h"
#include "UARTExtension.h"
#include "timing.h"
#include "keyboard.h"
#include "voltage_meter.h"

//used by SupplySystem
extern "C" void TIM2_IRQHandler(){
	supply::timer_isr();
}

extern "C" void EXTI9_5_IRQHandler() {
	supply::exti_isr();
}

extern "C" void TIM3_IRQHandler() {
	timing::handle_fine_timer_interrupt();
}

extern "C" void TIM4_IRQHandler() {
	timing::handle_coarse_timer_interrupt();
}

extern "C" void WWDG_IRQHandler() {
	__NOP();
}

extern "C" void UsageFault_Handler() {
	__NOP();
}

// ADC which measures battery voltage and sensor current of wired zones
extern "C" void ADC3_IRQHandler() {
	vmeter::isr();
}

extern "C" void USART2_IRQHandler() {
	sim900::handle_uart_interrupt();
}

extern "C" void DMA1_Channel7_IRQHandler() {
	sim900::handle_dma_interrupt();
}

extern "C" void EXTI0_IRQHandler() {
	sim900::handle_exti_interrupt();
}

//UARTExtension, Rx
extern "C" void USART3_IRQHandler(){
	if(SET == USART_GetITStatus(USART3, USART_IT_RXNE)){
		UARTExtension_Rx_IH();
		USART_ClearITPendingBit(USART3, USART_IT_RXNE);
	}
}

//UARTExtension, TxComplete
/*extern "C" void DMA1_Channel2_IRQHandler(){
	if(SET == DMA_GetITStatus(DMA1_IT_TC2)){
		UARTExtension_TxComplete_IH();
		DMA_ClearITPendingBit(DMA1_IT_TC2);
	}
}*/

//SPIExtension, SPI comm. finished
extern "C" void DMA2_Channel1_IRQHandler(){
	if(SET == DMA_GetITStatus(DMA2_IT_TC1)){
		SPIExtension_CommFinished_IH();
		DMA_ClearITPendingBit(DMA2_IT_TC1);
	}
}

//WirelessInterface, SPI comm. finished
extern "C" void DMA1_Channel4_IRQHandler(){
	if(SET == DMA_GetITStatus(DMA1_IT_TC4)){
		WirelessInterface_CommFinished_IH();
		DMA_ClearITPendingBit(DMA1_IT_TC4);
	}
}

// keyboard
extern "C" void EXTI15_10_IRQHandler() {
	keyboard::exti_isr();
}
