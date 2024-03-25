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
#include "GSMService.h"
#include "sim900_driver.h"
#include "WirelessInterface.h"
#include "UserInterface.h"
#include "VoltageMeter.h"
#include "SPIExtension.h"
#include "UARTExtension.h"
#include "timing.h"

extern "C" void SysTick_Handler(){
	xPortSysTickHandler();
}

//used by SupplySystem
extern "C" void TIM2_IRQHandler(){
	if(SET == TIM_GetITStatus(TIM2, TIM_IT_CC1)){
		SupplySystem_12VChannelsOvercurrent_Timer_IH();
		TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);
	}

	if(SET == TIM_GetITStatus(TIM2, TIM_IT_Update)){
		__NOP();
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}

extern "C" void TIM3_IRQHandler() {
	timing::handle_fine_timer_interrupt();
}

extern "C" void TIM4_IRQHandler() {
	timing::handleIrq();
}

extern "C" void WWDG_IRQHandler() {
	__NOP();
}

extern "C" void UsageFault_Handler() {
	__NOP();
}

//VoltageMeter, EOC
extern "C" void ADC3_IRQHandler(){
	if(SET == ADC_GetITStatus(ADC3, ADC_IT_EOC)){
		VoltageMeter_EndOfConversion_IH();
		ADC_ClearITPendingBit(ADC3, ADC_IT_EOC);
	}

}

extern "C" void USART2_IRQHandler() {
	sim900::handle_uart_interrupt();
}

extern "C" void DMA1_Channel7_IRQHandler() {
	sim900::handle_dma_interrupt();
}

//UARTExtension, Rx
extern "C" void USART3_IRQHandler(){
	if(SET == USART_GetITStatus(USART3, USART_IT_RXNE)){
		UARTExtension_Rx_IH();
		USART_ClearITPendingBit(USART3, USART_IT_RXNE);
	}
}

//UARTExtension, TxComplete
extern "C" void DMA1_Channel2_IRQHandler(){
	if(SET == DMA_GetITStatus(DMA1_IT_TC2)){
		UARTExtension_TxComplete_IH();
		DMA_ClearITPendingBit(DMA1_IT_TC2);
	}
}

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
