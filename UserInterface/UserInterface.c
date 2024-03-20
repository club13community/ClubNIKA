/*
 * UserInterface.c
 *
 *  Created on: 3 квіт. 2020 р.
 *      Author: MaxCm
 */

#include "UserInterface.h"
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"

#include "IRQPriorityConfig.h"

#include "ClockControl.h"

struct UserInterfaceContext{
	TaskHandle_t mainTask_handle;
	MessageBufferHandle_t msgIn, msgOut;
} ui_context;

#define turnOnLED() GPIO_SetBits(GPIOE, GPIO_Pin_6)
#define turnOffLED() GPIO_ResetBits(GPIOE, GPIO_Pin_6)

//TODO: implement with writes to registers
void  activateKeyboardRow(uint8_t row){
	//PC6 - R0; PC7 - R1; PD10 - R2; PD11 - R3
	GPIO_InitTypeDef pinInitStruct;
	//deactivate all
	pinInitStruct.GPIO_Mode=GPIO_Mode_IPU;
	pinInitStruct.GPIO_Pin=GPIO_Pin_6;
	GPIO_Init(GPIOC, &pinInitStruct);
	pinInitStruct.GPIO_Pin=GPIO_Pin_7;
	GPIO_Init(GPIOC, &pinInitStruct);

	pinInitStruct.GPIO_Pin=GPIO_Pin_10;
	GPIO_Init(GPIOD, &pinInitStruct);
	pinInitStruct.GPIO_Pin=GPIO_Pin_11;
	GPIO_Init(GPIOD, &pinInitStruct);

	//activate desired
	pinInitStruct.GPIO_Mode=GPIO_Mode_Out_PP;
	pinInitStruct.GPIO_Speed=GPIO_Speed_2MHz;
	switch(row & 0x3){
	case 0:
		GPIO_ResetBits(GPIOC, GPIO_Pin_6);
		pinInitStruct.GPIO_Pin=GPIO_Pin_6;
		GPIO_Init(GPIOC, &pinInitStruct);
		break;
	case 1:
		GPIO_ResetBits(GPIOC, GPIO_Pin_7);
		pinInitStruct.GPIO_Pin=GPIO_Pin_7;
		GPIO_Init(GPIOC, &pinInitStruct);
		break;
	case 2:
		GPIO_ResetBits(GPIOD, GPIO_Pin_10);
		pinInitStruct.GPIO_Pin=GPIO_Pin_10;
		GPIO_Init(GPIOD, &pinInitStruct);
		break;
	case 3:
		GPIO_ResetBits(GPIOD, GPIO_Pin_11);
		pinInitStruct.GPIO_Pin=GPIO_Pin_11;
		GPIO_Init(GPIOD, &pinInitStruct);
		break;
	}
}

//PD12-PD15 - C0-C3
#define getKeyboardColumns() ((uint8_t)(~(GPIO_ReadInputData(GPIOD)>>12) & 0x0F))

/*
 * Interrupt handlers
 */

void UserInterface_Timer_IH(){

}

void UserInterface_initState(){


}

void UserInterface_configPeripherals(){
	GPIO_InitTypeDef pinInitStruct;
	TIM_TimeBaseInitTypeDef timInitStruct;
	TIM_OCInitTypeDef timOcInitStruct;
	NVIC_InitTypeDef nvicInitStruct;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	//Config. pins for LCD
	pinInitStruct.GPIO_Speed=GPIO_Speed_10MHz;
	pinInitStruct.GPIO_Mode=GPIO_Mode_Out_PP;
	//PE4 - R/W, Read(1)/Write(0)
	pinInitStruct.GPIO_Pin=GPIO_Pin_4;
	GPIO_ResetBits(GPIOE, GPIO_Pin_4); //for certainty
	GPIO_Init(GPIOE, &pinInitStruct);
	//PE3 - R/S, Data(1)/Command(0)
	pinInitStruct.GPIO_Pin=GPIO_Pin_3;
	GPIO_Init(GPIOE, &pinInitStruct);
	//PE5 - Enable Line(latch command/data at pos. edge)
	GPIO_ResetBits(GPIOE, GPIO_Pin_5); //for certainty
	pinInitStruct.GPIO_Pin=GPIO_Pin_5;
	GPIO_Init(GPIOE, &pinInitStruct);
	//PC0-PC3 - data pins; leave pulled-up
	pinInitStruct.GPIO_Mode=GPIO_Mode_IPU;
	pinInitStruct.GPIO_Pin=GPIO_Pin_0;
	GPIO_Init(GPIOC, &pinInitStruct);
	pinInitStruct.GPIO_Pin=GPIO_Pin_1;
	GPIO_Init(GPIOC, &pinInitStruct);
	pinInitStruct.GPIO_Pin=GPIO_Pin_2;
	GPIO_Init(GPIOC, &pinInitStruct);
	pinInitStruct.GPIO_Pin=GPIO_Pin_3;
	GPIO_Init(GPIOC, &pinInitStruct);

	//PE6 - LED
	pinInitStruct.GPIO_Mode=GPIO_Mode_Out_PP;
	pinInitStruct.GPIO_Speed=GPIO_Speed_10MHz;
	pinInitStruct.GPIO_Pin=GPIO_Pin_6;
	GPIO_ResetBits(GPIOE, GPIO_Pin_6); //for certainty
	GPIO_Init(GPIOE, &pinInitStruct);

	//Config. pins for keyboard
	//PC6 - R0; PC7 - R1; PD10 - R2; PD11 - R3
	pinInitStruct.GPIO_Mode=GPIO_Mode_IPU;
	pinInitStruct.GPIO_Pin=GPIO_Pin_6;
	GPIO_Init(GPIOC, &pinInitStruct);
	pinInitStruct.GPIO_Pin=GPIO_Pin_7;
	GPIO_Init(GPIOC, &pinInitStruct);

	pinInitStruct.GPIO_Pin=GPIO_Pin_10;
	GPIO_Init(GPIOD, &pinInitStruct);
	pinInitStruct.GPIO_Pin=GPIO_Pin_11;
	GPIO_Init(GPIOD, &pinInitStruct);

	//PD12-PD15 - C0-C3
	pinInitStruct.GPIO_Mode=GPIO_Mode_IN_FLOATING;
	pinInitStruct.GPIO_Pin=GPIO_Pin_12;
	GPIO_Init(GPIOD, &pinInitStruct);
	pinInitStruct.GPIO_Pin=GPIO_Pin_13;
	GPIO_Init(GPIOD, &pinInitStruct);
	pinInitStruct.GPIO_Pin=GPIO_Pin_14;
	GPIO_Init(GPIOD, &pinInitStruct);
	pinInitStruct.GPIO_Pin=GPIO_Pin_15;
	GPIO_Init(GPIOD, &pinInitStruct);

	//Config. timer
	int32_t tmrFreq=getPeripheralClockFrequencyKHz(TIM3_BASE);
	if(tmrFreq<=0){
		//report critical problem
		//TODO: implement correctly
		tmrFreq=1000;
	}
	timInitStruct.TIM_Prescaler = 0;
	timInitStruct.TIM_Period = tmrFreq-1; //get 1ms
	timInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	timInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	timInitStruct.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM3, &timInitStruct);

	timOcInitStruct.TIM_OCMode = TIM_OCMode_Timing;
	//timOcInitStruct.the rest = no effect
	TIM_OC1Init(TIM3, &timOcInitStruct);
	TIM_SetCompare1(TIM3, tmrFreq>>1);
	TIM_ITConfig(TIM3, TIM_IT_CC1, ENABLE);
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	nvicInitStruct.NVIC_IRQChannel = TIM3_IRQn;
	nvicInitStruct.NVIC_IRQChannelCmd = DISABLE;
	nvicInitStruct.NVIC_IRQChannelPreemptionPriority = USERINTERFACE_TIMER_PRIORITY;
	nvicInitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvicInitStruct);

	//TIM_Cmd(TIM3, ENABLE);
}

void UserInterface_mainTask(void *pvParameters){
	//turnOnLED();
	uint8_t col;
	activateKeyboardRow(0);
	while(1){
		col=getKeyboardColumns();
		taskYIELD();
	}
}

TaskHandle_t UserInterface_registerInOS(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	//Save message channels
	ui_context.msgIn=msgIn;
	ui_context.msgOut=msgOut;

	static const char mainTask_name[configMAX_TASK_NAME_LEN]="UserInterface";
	static StaticTask_t mainTask_TCB;
	static StackType_t mainTask_stackBuffer[32]; //length=mainTask_stackDepth

	// Create tasks
	ui_context.mainTask_handle=xTaskCreateStatic(UserInterface_mainTask,
			mainTask_name,
			32, //mainTask_stackDepth
			(void *)0,
			1,
			mainTask_stackBuffer,
			&mainTask_TCB);

	return ui_context.mainTask_handle;
}

TaskHandle_t UserInterface_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	TaskHandle_t mainTask_handle;
	UserInterface_initState();
	UserInterface_configPeripherals();
	mainTask_handle=UserInterface_registerInOS(msgIn, msgOut);
	return mainTask_handle;
}
