#include "ClockControl.h"
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_flash.h"

#define CLOCK_CONF_HSI		((uint8_t)0)
#define CLOCK_CONF_SLOW		((uint8_t)1)
#define CLOCK_CONF_NOMINAL	((uint8_t)2)
#define CLOCK_CONF_FAST		((uint8_t)3)
#define CLOCK_CONF_UNKNOWN	((uint8_t)4)
#define CLOCK_CONF_MASK		((uint8_t)0x7)
#define CLOCK_ERROR_FLAG	((uint8_t)0x80)

volatile uint8_t clockStatus=CLOCK_CONF_HSI;
const int32_t AHBClockValues[]={8000, 8000, 12000, 24000};
const int32_t APB1ClockValues[]={8000, 8000, 12000, 12000};
const int32_t APB2ClockValues[]={8000, 8000, 12000, 12000};
const int32_t APB1TimerClockValues[]={8000, 8000, 12000, 24000};
const int32_t APB2TimerClockValues[]={8000, 8000, 12000, 24000};
const int32_t APB2AdcClockValues[]={2000, 2000, 2000, 2000};

//TODO: move this somewhere - ClockControl.h should not be included to other services
int32_t getPeripheralClockFrequencyKHz(uint32_t periphBaseAddr){
	uint8_t clockConf=clockStatus & CLOCK_CONF_MASK;

	if(CLOCK_CONF_UNKNOWN == clockConf){
		return -1;
	}

	switch(periphBaseAddr){
	case TIM2_BASE: return APB1TimerClockValues[clockConf];
	case TIM3_BASE: return APB1TimerClockValues[clockConf];
	case TIM4_BASE: return APB1TimerClockValues[clockConf];
	case TIM5_BASE: return APB1TimerClockValues[clockConf];
	case TIM6_BASE: return APB1TimerClockValues[clockConf];
	}
	return -1;
}

//AHB clock=8MHz(HSI), APB1 clock=8MHz(HSI), APB2 clock=8MHz(HSI), ADC clock=2MHz
uint8_t clockFromHSI(){
	uint32_t timeout = 0;

	//Switch to HSI clock (HSI is always running)
	RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
	timeout=24*10; //more, that 10us at "fast clock config"
	do{
		timeout--;
	} while(RCC_CFGR_SWS_HSI != RCC_GetSYSCLKSource() && 0 < timeout);
	if(RCC_CFGR_SWS_HSI != RCC_GetSYSCLKSource()){
		//hardware fault, critical - can not proceed
		return CLOCK_ERROR_FLAG | CLOCK_CONF_UNKNOWN;
	}
	RCC_PLLCmd(DISABLE);
	RCC_HSEConfig(RCC_HSE_OFF);

	FLASH_SetLatency(FLASH_Latency_0);
	RCC_PCLK1Config(RCC_HCLK_Div1);
	RCC_PCLK2Config(RCC_HCLK_Div1);
	RCC_HCLKConfig(RCC_SYSCLK_Div1);
	RCC_ADCCLKConfig(RCC_PCLK2_Div4);
	return CLOCK_CONF_HSI;
}

//fastClock==0 -> AHB clock=12MHz(HSE+PLL), APB1 clock=12MHz(HSE+PLL), APB2 clock=12MHz(HSE+PLL), ADC clock=2MHz
//fastClock!=0 -> AHB clock=24MHz(HSE+PLL), APB1 clock=12MHz(HSE+PLL), APB2 clock=12MHz(HSE+PLL), ADC clock=2MHz
uint8_t setClockFromPLL(uint32_t fastClock){
	uint32_t timeout = 0;

	//Switch to HSI clock (HSI is always running)
	RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
	timeout=24*10; //more, that 10us at "fast clock config"
	do{
		timeout--;
	} while(RCC_CFGR_SWS_HSI != RCC_GetSYSCLKSource() && 0 < timeout);
	if(RCC_CFGR_SWS_HSI != RCC_GetSYSCLKSource()){
		//hardware fault, critical - can not proceed
		return CLOCK_ERROR_FLAG | CLOCK_CONF_UNKNOWN;
	}
	//disable PLL, will be enabled later with specified mult.
	RCC_PLLCmd(DISABLE);

	//Enable HSE
	RCC_HSEConfig(RCC_HSE_ON);
	timeout=8*400; //more, that 400us
	do
	{
		timeout--;
	} while(SET != RCC_GetFlagStatus(RCC_FLAG_HSERDY) && 0 < timeout);
	if (SET != RCC_GetFlagStatus(RCC_FLAG_HSERDY)){
		//hardware fault, limited functionality - can proceed on HSI
		RCC_HSEConfig(RCC_HSE_OFF);
		FLASH_SetLatency(FLASH_Latency_0);
		RCC_PCLK1Config(RCC_HCLK_Div1);
		RCC_PCLK2Config(RCC_HCLK_Div1);
		RCC_HCLKConfig(RCC_SYSCLK_Div1);
		RCC_ADCCLKConfig(RCC_PCLK2_Div4);
		return CLOCK_ERROR_FLAG | CLOCK_CONF_HSI;
	}

	//Enable PLL
	RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_6);
	RCC_PLLCmd(ENABLE);
	timeout=8*400; //more, that 400us
	do
	{
		timeout--;
	} while(SET != RCC_GetFlagStatus(RCC_FLAG_PLLRDY) && 0 < timeout);
	if(SET != RCC_GetFlagStatus(RCC_FLAG_PLLRDY)){
		//hardware fault, limited functionality - can proceed on HSI
		RCC_PLLCmd(DISABLE); //disable PLL
		RCC_HSEConfig(RCC_HSE_OFF); //disable HSE
		FLASH_SetLatency(FLASH_Latency_0);
		RCC_PCLK1Config(RCC_HCLK_Div1);
		RCC_PCLK2Config(RCC_HCLK_Div1);
		RCC_HCLKConfig(RCC_SYSCLK_Div1);
		RCC_ADCCLKConfig(RCC_PCLK2_Div4);
		return CLOCK_ERROR_FLAG | CLOCK_CONF_HSI;
	}
	FLASH_SetLatency(FLASH_Latency_1);
	if(0==fastClock){
		RCC_PCLK1Config(RCC_HCLK_Div1);
		RCC_PCLK2Config(RCC_HCLK_Div1);
		RCC_HCLKConfig(RCC_SYSCLK_Div4);
		RCC_ADCCLKConfig(RCC_PCLK2_Div6);
	} else {
		RCC_PCLK1Config(RCC_HCLK_Div2);
		RCC_PCLK2Config(RCC_HCLK_Div2);
		RCC_HCLKConfig(RCC_SYSCLK_Div2);
		RCC_ADCCLKConfig(RCC_PCLK2_Div6);
	}

	RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
	timeout=8*10; //more, that 10us
	do{
		timeout--;
	}while(RCC_CFGR_SWS_PLL != RCC_GetSYSCLKSource() && 0 < timeout);
	if(RCC_CFGR_SWS_PLL != RCC_GetSYSCLKSource()){
		//hard fault, critical - not certain if HSI or HSE+PLL, if clock is stable
		//try to switch back to HSI, do not touch the rest
		RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
		return CLOCK_ERROR_FLAG | CLOCK_CONF_UNKNOWN;
	}
	if(fastClock){
		return CLOCK_CONF_FAST;
	}
	return CLOCK_CONF_NOMINAL;
}

//AHB clock=8MHz(HSE), APB1 clock=8MHz(HSE), APB2 clock=8MHz(HSE), ADC clock=2MHz
uint8_t setSlowClock(){
	uint32_t timeout = 0;

	//Switch to HSI clock (HSI is always running)
	RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
	timeout=24*10; //more, that 10us at "fast clock config"
	do{
		timeout--;
	} while(RCC_CFGR_SWS_HSI != RCC_GetSYSCLKSource() && 0 < timeout);
	if(RCC_CFGR_SWS_HSI != RCC_GetSYSCLKSource()){
		//hardware fault, critical - can not proceed
		return CLOCK_ERROR_FLAG | CLOCK_CONF_UNKNOWN;
	}
	//disable PLL
	RCC_PLLCmd(DISABLE);

	//Enable HSE
	RCC_HSEConfig(RCC_HSE_ON);
	timeout=8*400; //more, that 400us
	do
	{
		timeout--;
	} while(SET != RCC_GetFlagStatus(RCC_FLAG_HSERDY) && 0 < timeout);
	if (SET != RCC_GetFlagStatus(RCC_FLAG_HSERDY)){
		//hardware fault, limited functionality - can proceed on HSI
		RCC_HSEConfig(RCC_HSE_OFF);
		FLASH_SetLatency(FLASH_Latency_0);
		RCC_PCLK1Config(RCC_HCLK_Div1);
		RCC_PCLK2Config(RCC_HCLK_Div1);
		RCC_HCLKConfig(RCC_SYSCLK_Div1);
		RCC_ADCCLKConfig(RCC_PCLK2_Div4);
		return CLOCK_ERROR_FLAG | CLOCK_CONF_HSI;
	} else {
		FLASH_SetLatency(FLASH_Latency_0);
		RCC_PCLK1Config(RCC_HCLK_Div1);
		RCC_PCLK2Config(RCC_HCLK_Div1);
		RCC_HCLKConfig(RCC_SYSCLK_Div1);
		RCC_SYSCLKConfig(RCC_SYSCLKSource_HSE);
		RCC_ADCCLKConfig(RCC_PCLK2_Div4);
		timeout=8*10; //more, that 10us
		do{
			timeout--;
		}while(RCC_CFGR_SWS_HSE != RCC_GetSYSCLKSource() && 0 < timeout);
		if(RCC_CFGR_SWS_HSE != RCC_GetSYSCLKSource()){
			//hard fault, critical - not certain if HSI or HSE, if clock is stable
			//try to switch back to HSI, do not touch the rest
			RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
			return CLOCK_ERROR_FLAG | CLOCK_CONF_UNKNOWN;
		}
	}
	return CLOCK_CONF_SLOW;
}


void setNominalClock(){
	clockStatus=setClockFromPLL(0);
}


void setFastClock(){
	clockStatus=setClockFromPLL(1);
}

//config clock after start-up
void ClockControl_StartUpInit(){
	clockStatus=setClockFromPLL(0);
}

struct ClockControlContext{
	MessageBufferHandle_t msgIn, msgOut;
} clkCtrl_context;


void ClockControl_mainTask(void *pvParameters){
	while(1){
		taskYIELD();
	}
}

void ClockControl_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	TaskHandle_t mainTask_handle;
	static const char mainTask_name[configMAX_TASK_NAME_LEN]="ClockControl";
	static StaticTask_t mainTask_TCB;
	static StackType_t mainTask_stackBuffer[32]; //length=mainTask_stackDepth

	clkCtrl_context.msgIn=msgIn;
	clkCtrl_context.msgOut=msgOut;

	/*mainTask_handle=xTaskCreateStatic(ClockControl_mainTask,
			mainTask_name,
			32, //mainTask_stackDepth
			(void *)0,
			1,
			mainTask_stackBuffer,
			&mainTask_TCB);*/

	//return mainTask_handle;
}
