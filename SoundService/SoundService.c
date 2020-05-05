/*
 * SoundService.h
 *
 *  Created on: 3 квіт. 2020 р.
 *      Author: MaxCm
 */
#include "SoundService.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_dac.h"
#include "stm32f10x_adc.h"

#include "ClockControl.h"

struct SoundServiceContext{
	TaskHandle_t mainTask_handle;
	MessageBufferHandle_t msgIn, msgOut;

} sound_context;


#define disableSpeaker() GPIO_SetBits(GPIOE, GPIO_Pin_10)
#define enableSpeaker() GPIO_ResetBits(GPIOE, GPIO_Pin_10)

#define playerToSpeaker() GPIO_ResetBits(GPIOE, GPIO_Pin_12)
#define gsmToSpeaker() GPIO_SetBits(GPIOE, GPIO_Pin_12)

#define disableGsmMicrophone() GPIO_ResetBits(GPIOE, GPIO_Pin_11)
#define enableGsmMicrophone() GPIO_SetBits(GPIOE, GPIO_Pin_11)

volatile uint32_t acquisitionBuffer[128];
volatile uint32_t playBuffer1[128];
volatile uint32_t playBuffer2[128];

volatile uint32_t SoundService_flags;
#define FLAG_PLAYER_FAILURE		((uint32_t)(1<<0))

void SoundService_initState(){

}

void SoundService_configPeripherals(){
	/*RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	GPIO_InitTypeDef pinInitStruct;
	//PE10 - "mute speaker"
	pinInitStruct.GPIO_Mode = GPIO_Mode_Out_OD;
	pinInitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	pinInitStruct.GPIO_Pin = GPIO_Pin_10;
	disableSpeaker();
	GPIO_Init(GPIOE, &pinInitStruct);

	pinInitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	pinInitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	//PE11 - "enable GSM microphone"
	pinInitStruct.GPIO_Pin = GPIO_Pin_11;
	GPIO_Init(GPIOE, &pinInitStruct);
	//PE12 - "speaker source selection"
	pinInitStruct.GPIO_Pin = GPIO_Pin_11;
	GPIO_Init(GPIOE, &pinInitStruct);
	//PA4 - "player output", DAC, OUT1
	DAC_InitTypeDef dacInitStruct;
	dacInitStruct.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
	dacInitStruct.DAC_Trigger = DAC_Trigger_T4_TRGO;
	dacInitStruct.DAC_WaveGeneration = DAC_WaveGeneration_None;
	DAC_Init(DAC_Channel_1, &dacInitStruct);


	//DAC_Cmd(DAC_Channel_1, ENABLE);

	//PC5 - "microphone input", ADC2, AIN15
	pinInitStruct.GPIO_Mode = GPIO_Mode_AIN;
	pinInitStruct.GPIO_Pin = GPIO_Pin_5;
	GPIO_Init(GPIOC, &pinInitStruct);


	TIM_TimeBaseInitTypeDef timInitStruct;
	int32_t tmrFreq;
	tmrFreq=getPeripheralClockFrequencyKHz(TIM4_BASE);
	if(tmrFreq<0){
		SoundService_flags |= FLAG_PLAYER_FAILURE; //no race condition
		return;
	}
	timInitStruct.TIM_Prescaler = tmrFreq>>5; //get 1/32 ms
	timInitStruct.TIM_Period = 0; //will be set later
	timInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	timInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	timInitStruct.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM4, &timInitStruct);
	TIM_SelectOutputTrigger(TIM4, TIM_TRGOSource_Update); //update player DAC

	TIM_OCInitTypeDef timOcInitStruct;
	timOcInitStruct.TIM_OCMode = TIM_OCMode_Timing;
	//timOcInitStruct.the rest = no effect
	TIM_OC4Init(TIM4, &timOcInitStruct);
	//TIM_SetCompare4(TIM4, 0); //will be set later
	//TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);
	//TIM_Cmd(TIM2, ENABLE);*/

}

void SoundService_mainTask(void *pvParameters){
	while(1){
		taskYIELD();
	}
}

TaskHandle_t SoundService_registerInOS(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	sound_context.msgIn=msgIn;
	sound_context.msgOut=msgOut;

	static const char mainTask_name[configMAX_TASK_NAME_LEN]="SoundService";
	static StaticTask_t mainTask_TCB;
	static StackType_t mainTask_stackBuffer[32]; //length=mainTask_stackDepth

	// Create tasks
	sound_context.mainTask_handle=xTaskCreateStatic(SoundService_mainTask,
			mainTask_name,
			32, //mainTask_stackDepth
			(void *)0,
			1,
			mainTask_stackBuffer,
			&mainTask_TCB);

	return sound_context.mainTask_handle;
}

TaskHandle_t SoundService_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	TaskHandle_t mainTask_handle;
	SoundService_initState();
	SoundService_configPeripherals();
	mainTask_handle=SoundService_registerInOS(msgIn, msgOut);
	return mainTask_handle;
}

