/*
 * interrupts.c
 *
 *  Created on: 30 бер. 2020 р.
 *      Author: MaxCm
 */
#include "stm32f10x_exti.h"
#include "stm32f10x_tim.h"
#include "SupplySystem.h"
#include "sim900_isr.h"
#include "UserInterface.h"
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

// keyboard
extern "C" void EXTI15_10_IRQHandler() {
	keyboard::exti_isr();
}
