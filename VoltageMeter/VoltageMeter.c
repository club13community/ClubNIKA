/*
 * VoltageMeter.c
 *
 *  Created on: 4 трав. 2020 р.
 *      Author: MaxCm
 */
#include "VoltageMeter.h"

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_adc.h"

#include "IRQPriorityConfig.h"

struct VoltageMeterContext{
	MessageBufferHandle_t msgIn, msgOut;
	uint8_t flags;
	uint16_t adcData;
} voltMet_context;

#define FLAG_ConversionIsActive	((uint8_t)(1<<0))

#define selectSensorChannel(channel) do{\
									GPIO_ResetBits(GPIOE, (~channel) & 0x0007)\
									GPIO_SetBits(GPIOE, channel & 0x0007);while(0)

//to measure current, that sinks into sensor
#define selectSensorSinkCurrent() GPIO_SetBits(GPIOB, GPIO_Pin_9)
//to measure current, that sources from sensor
#define selectSensorSourceCurrent() GPIO_ResetBits(GPIOB, GPIO_Pin_9)

#define ADC_Channel_SensorSinkCurrent 	ADC_Channel_1
#define ADC_Channel_SensorSourceCurrent ADC_Channel_0
#define ADC_Channel_BatteryVoltage		ADC_Channel_2

#define setADCChannel(adcChannel) ADC_RegularChannelConfig(ADC3, adcChannel, 0, ADC_SampleTime_239Cycles5);

#define startConversionFromTask() do{\
									taskENTER_CRITICAL();\
									voltMet_context.flags |= FLAG_ConversionIsActive;\
									taskEXIT_CRITICAL();\
									ADC_SoftwareStartConvCmd(ADC3, ENABLE);while(0)

#define isCalibrationActive() (SET==ADC_GetCalibrationStatus(ADC3))?(1):(0)

/*
 * Interrupt handler
 */
void VoltageMeter_EndOfConversion_IH(){
	voltMet_context.adcData=ADC_GetConversionValue(ADC3);
	voltMet_context.flags &= ~FLAG_ConversionIsActive; //no racing
}


void VoltageMeter_mainTask(void *pvParameters){
	while(1){

	}
}

void VoltageMeter_initState(){
	voltMet_context.flags=0;
}

void VoltageMeter_configPeripherals(){
	GPIO_InitTypeDef pinInitStruct;
	NVIC_InitTypeDef nvicInitStruct;
	ADC_InitTypeDef adcInitStruct;
	uint8_t i;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC3, ENABLE);

	//Config pins
	pinInitStruct.GPIO_Mode=GPIO_Mode_Out_PP;
	pinInitStruct.GPIO_Speed=GPIO_Speed_2MHz;
	//PE0 - Sensor channel select [0]
	pinInitStruct.GPIO_Pin=GPIO_Pin_0;
	GPIO_Init(GPIOE, &pinInitStruct);
	//PE1 - Sensor channel select [1]
	pinInitStruct.GPIO_Pin=GPIO_Pin_1;
	GPIO_Init(GPIOE, &pinInitStruct);
	//PE2 - Sensor channel select [2]
	pinInitStruct.GPIO_Pin=GPIO_Pin_2;
	GPIO_Init(GPIOE, &pinInitStruct);
	//PB9 - Sensor current direction
	pinInitStruct.GPIO_Pin=GPIO_Pin_9;
	GPIO_Init(GPIOB, &pinInitStruct);

	pinInitStruct.GPIO_Mode=GPIO_Mode_AIN;
	//PA0 - sensor source current(measure sink current)
	pinInitStruct.GPIO_Pin=GPIO_Pin_0;
	GPIO_Init(GPIOA, &pinInitStruct);
	//PA1 - sensor sink current(measure source current)
	pinInitStruct.GPIO_Pin=GPIO_Pin_1;
	GPIO_Init(GPIOA, &pinInitStruct);
	//PA2 - battery voltage
	pinInitStruct.GPIO_Pin=GPIO_Pin_2;
	GPIO_Init(GPIOA, &pinInitStruct);

	//Config ADC
	adcInitStruct.ADC_Mode=ADC_Mode_Independent;
	adcInitStruct.ADC_ScanConvMode=DISABLE;
	adcInitStruct.ADC_ContinuousConvMode=DISABLE;
	adcInitStruct.ADC_DataAlign=ADC_DataAlign_Right;
	adcInitStruct.ADC_ExternalTrigConv=ADC_ExternalTrigConv_None;
	adcInitStruct.ADC_NbrOfChannel=1;
	ADC_Init(ADC3, &adcInitStruct);

	//Config. End Of Conversion interrupt
	ADC_ITConfig(ADC3, ADC_IT_EOC, ENABLE);

	nvicInitStruct.NVIC_IRQChannel = ADC3_IRQn;
	nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
	nvicInitStruct.NVIC_IRQChannelPreemptionPriority = VOLTAGEMETER_EOC_PRIORITY;
	nvicInitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvicInitStruct);

	ADC_Cmd(ADC3, ENABLE); //wake from power-down
	//wait more than 1us with fastest AHB clock; for power-up
	for(i=0; i<6; i++){
		__NOP();
		__NOP();
		__NOP();
		__NOP();
	}
	//wait more that 2 ADC clock cycles with fastest AHB
	for(i=0; i<6; i++){
		__NOP();
		__NOP();
		__NOP();
		__NOP();
	}
	//start self calibration
	ADC_StartCalibration(ADC3);
}

TaskHandle_t VoltageMeter_registerInOS(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	TaskHandle_t mainTask_handle;
	//Save message channels
	voltMet_context.msgIn=msgIn;
	voltMet_context.msgOut=msgOut;

	//Create tasks
	static const char mainTask_name[configMAX_TASK_NAME_LEN]="VoltageMeter";
	static StaticTask_t mainTask_TCB;
	static StackType_t mainTask_stackBuffer[32]; //length=mainTask_stackDepth

	mainTask_handle=xTaskCreateStatic(VoltageMeter_mainTask,
			mainTask_name,
			32, //mainTask_stackDepth
			(void *)0,
			1,
			mainTask_stackBuffer,
			&mainTask_TCB);

	return mainTask_handle;
}

TaskHandle_t VoltageMeter_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	TaskHandle_t mainTask_handle;
	VoltageMeter_initState();
	VoltageMeter_configPeripherals();
	mainTask_handle=VoltageMeter_registerInOS(msgIn, msgOut);
	return mainTask_handle;
}

