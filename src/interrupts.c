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
#include "WirelessInterface.h"
#include "UserInterface.h"
#include "VoltageMeter.h"
#include "SPIExtension.h"
#include "UARTExtension.h"

void SysTick_Handler(){
	xPortSysTickHandler();
}

//used by SupplySystem
void TIM2_IRQHandler(){
	if(SET == TIM_GetITStatus(TIM2, TIM_IT_CC1)){
		SupplySystem_12VChannelsOvercurrent_Timer_IH();
		TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);
	}

	if(SET == TIM_GetITStatus(TIM2, TIM_IT_Update)){
		__NOP();
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}

//VoltageMeter, EOC
void ADC3_IRQHandler(){
	if(SET == ADC_GetITStatus(ADC3, ADC_IT_EOC)){
		VoltageMeter_EndOfConversion_IH();
		ADC_ClearITPendingBit(ADC3, ADC_IT_EOC);
	}

}

//UARTExtension, Rx
void USART3_IRQHandler(){
	if(SET == USART_GetITStatus(USART3, USART_IT_RXNE)){
		UARTExtension_Rx_IH();
		USART_ClearITPendingBit(USART3, USART_IT_RXNE);
	}
}

//UARTExtension, TxComplete
void DMA1_Channel2_IRQHandler(){
	if(SET == DMA_GetITStatus(DMA1_IT_TC2)){
		UARTExtension_TxComplete_IH();
		DMA_ClearITPendingBit(DMA1_IT_TC2);
	}
}

//SPIExtension, SPI comm. finished
void DMA2_Channel1_IRQHandler(){
	if(SET == DMA_GetITStatus(DMA2_IT_TC1)){
		SPIExtension_CommFinished_IH();
		DMA_ClearITPendingBit(DMA2_IT_TC1);
	}
}

//WirelessInterface, SPI comm. finished
void DMA1_Channel4_IRQHandler(){
	if(SET == DMA_GetITStatus(DMA1_IT_TC4)){
		WirelessInterface_CommFinished_IH();
		DMA_ClearITPendingBit(DMA1_IT_TC4);
	}
}

//UserInterface
void TIM3_IRQHandler(){
	if(SET == TIM_GetITStatus(TIM3, TIM_IT_CC1)){
		UserInterface_Timer_IH();
		TIM_ClearITPendingBit(TIM3, TIM_IT_CC1);
	}

	if(SET == TIM_GetITStatus(TIM3, TIM_IT_Update)){
		UserInterface_Timer_IH();
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	}
}

