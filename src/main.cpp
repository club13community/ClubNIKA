/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "croutine.h"
#include "timing.h"
#include "ClockControl.h"
#include "SupplySystem.h"
#include "MessageRouter.h"
#include "wired_zones.h"
#include "voltage_meter.h"
#include "GSMService.h"
#include "WirelessInterface.h"
#include "UserInterface.h"
#include "SoundService.h"
#include "UARTExtension.h"
#include "SPIExtension.h"
#include "I2CExtension.h"
#include "Message.h"
#include "flash.h"
#include "test.h"
#include "UserInterface.h"

struct TaskHandleRegister{
	TaskHandle_t stkMon_handle;
	TaskHandle_t clkCtrl_handle;
	TaskHandle_t supSys_handle;
	TaskHandle_t msg_handle;
	TaskHandle_t wsens_handle;
	TaskHandle_t voltMet_handle;
	TaskHandle_t gsm_handle;
	TaskHandle_t wless_handle;
	TaskHandle_t ui_handle;
	TaskHandle_t sound_handle;
	TaskHandle_t uartExt_handle;
	TaskHandle_t spiExt_handle;
	TaskHandle_t i2cExt_handle;
} taskHandleRegister;

void clearTaskHandleRegister(){
	taskHandleRegister.stkMon_handle=0;
	taskHandleRegister.clkCtrl_handle=0;
	taskHandleRegister.supSys_handle=0;
	taskHandleRegister.msg_handle=0;
	taskHandleRegister.wsens_handle=0;
	taskHandleRegister.voltMet_handle=0;
	taskHandleRegister.gsm_handle=0;
	taskHandleRegister.wless_handle=0;
	taskHandleRegister.ui_handle=0;
	taskHandleRegister.sound_handle=0;
	taskHandleRegister.uartExt_handle=0;
	taskHandleRegister.spiExt_handle=0;
	taskHandleRegister.i2cExt_handle=0;
}

#ifdef DEBUG
TaskHandle_t StackMonitor_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut);
#endif

volatile bool done = false;

void do_done() {
	done = true;
}

// float div = 170u, mult = 60u, sub = 75u
extern "C" int main(void)
{
	SystemInit();
	ClockControl_StartUpInit();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	timing::config_coarse_timer();
	timing::config_fine_timer();
	vmeter::init_periph();

	//test_timer_wait();
	//test_timer_invoke_once();
	//test_timer_invoke_repeatedly_and_stop();
	//test_timer_start_blink_while_delay();

	/*RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	ADC1->SMPR1 = ADC_SMPR1_SMP17_2 | ADC_SMPR1_SMP17_1;
	ADC1->SQR1 = 2U << 20;
	ADC1->SQR3 = 17U | 16U << 5 | 17U << 10;
	ADC1->JSQR = 17U << 5 | 16U << 10 | 17U << 15 | 2U << 20;
	ADC1->CR1 = ADC_CR1_JDISCEN | ADC_CR1_DISCNUM_0;
	ADC1->CR2 = ADC_CR2_TSVREFE | ADC_CR2_ADON
			| ADC_CR2_JEXTTRIG | ADC_CR2_JEXTSEL_2 | ADC_CR2_JEXTSEL_1 | ADC_CR2_JEXTSEL_0;
	for (uint8_t i = 0; i < 200; i++);*/

	/*ADC1->CR2 = ADC1->CR2;
	while ((ADC1->SR & (ADC_SR_JEOC | ADC_SR_EOC)) != ADC_SR_EOC);
	ADC1->SR = ~ADC_SR_EOC;
	uint16_t val1 = ADC1->DR;

	ADC1->CR2 = ADC1->CR2;
	while ((ADC1->SR & (ADC_SR_JEOC | ADC_SR_EOC)) != ADC_SR_EOC);
	ADC1->SR = ~ADC_SR_EOC;
	uint16_t val2 = ADC1->DR;

	ADC1->CR2 = ADC1->CR2;
	while ((ADC1->SR & (ADC_SR_JEOC | ADC_SR_EOC)) != ADC_SR_EOC);
	ADC1->SR = ~ADC_SR_EOC;
	uint16_t val3 = ADC1->DR;*/

	/*ADC1->CR2 |= ADC_CR2_JSWSTART;
	while ((ADC1->SR & (ADC_SR_JEOC | ADC_SR_EOC)) != (ADC_SR_EOC | ADC_SR_JEOC));
	ADC1->SR = ~ADC_SR_EOC & ~ADC_SR_JEOC;
	uint16_t val1 = ADC1->JDR1;

	ADC1->CR2 |= ADC_CR2_JSWSTART;
	while ((ADC1->SR & (ADC_SR_JEOC | ADC_SR_EOC)) != (ADC_SR_EOC | ADC_SR_JEOC));
	ADC1->SR = ~ADC_SR_EOC & ~ADC_SR_JEOC;
	uint16_t val2 = ADC1->JDR2;

	ADC1->CR2 |= ADC_CR2_JSWSTART;
	while ((ADC1->SR & (ADC_SR_JEOC | ADC_SR_EOC)) != (ADC_SR_EOC | ADC_SR_JEOC));
	ADC1->SR = ~ADC_SR_EOC & ~ADC_SR_JEOC;
	uint16_t val3 = ADC1->DR;

	__NOP();*/

	flash::init();
	gsm_service::init_periph();
	sound_service::init_periph();
	user_interface::init_periph();
	wired_zones::init_periph();

	MessageRouter_StartUpInit();
	clearTaskHandleRegister();

	ClockControl_Launch(getMessageBuffer_ClockControlDataIn(), getMessageBuffer_ClockControlDataOut());
	SupplySystem_Launch(getMessageBuffer_SupplySystemDataIn(), getMessageBuffer_SupplySystemDataOut());
	vmeter::start();
	user_interface::start();
	wired_zones::start();
	//taskHandleRegister.wsens_handle=WiredSensorMonitor_Launch(getMessageBuffer_WiredSensorMonitorDataIn(), getMessageBuffer_WiredSensorMonitorDataOut());
	//taskHandleRegister.voltMet_handle=VoltageMeter_Launch(getMessageBuffer_VoltageMeterDataIn(), getMessageBuffer_VoltageMeterDataOut());
	//taskHandleRegister.wless_handle=WirelessInterface_Launch(getMessageBuffer_WirelessInterfaceDataIn(), getMessageBuffer_WirelessInterfaceDataOut());
	//taskHandleRegister.ui_handle=UserInterface_Launch(getMessageBuffer_UserInterfaceDataIn(), getMessageBuffer_UserInterfaceDataOut());
	//taskHandleRegister.uartExt_handle=UARTExtension_Launch(getMessageBuffer_UARTExtensionDataIn(), getMessageBuffer_UARTExtensionDataOut());
	//taskHandleRegister.spiExt_handle=SPIExtension_Launch(getMessageBuffer_SPIExtensionDataIn(), getMessageBuffer_SPIExtensionDataOut());
	//taskHandleRegister.i2cExt_handle=I2CExtension_Launch(getMessageBuffer_I2CExtensionDataIn(), getMessageBuffer_I2CExtensionDataOut());

	//taskHandleRegister.msg_handle=MessageRouter_Launch();

	/*uint8_t mem[5] = {1, 2, 3, 4, 5};
	uint8_t buf[5] = {0xA, 0xA, 0xA, 0xA, 0xA};
	using namespace flash;
	init();

	done = false;
	memory_to_buffer(0, Buffer::B2, do_done);
	while (!done);

	done = false;
	read_memory({.page = 0, .byte = 0}, 4, mem, do_done);
	while (!done);

	done = false;
	read_buffer(Buffer::B2, 0, 4, buf, do_done);
	while(!done);

	__NOP();*/

	//wireless
	//File system
	//idle task
	//timer task

#ifdef DEBUG
	//taskHandleRegister.stkMon_handle=StackMonitor_Launch(getMessageBuffer_StackMonitorDataIn(), getMessageBuffer_StackMonitorDataOut());
#endif

	vTaskStartScheduler();
}


extern "C" void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize ){
	static StaticTask_t xIdleTaskTCB;
	static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

	*ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
	*ppxIdleTaskStackBuffer = uxIdleTaskStack;
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

extern "C" void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t **ppxTimerTaskStackBuffer,
                                     uint32_t *pulTimerTaskStackSize ){
	static StaticTask_t xTimerTaskTCB;
	static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

	*ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
	*ppxTimerTaskStackBuffer = uxTimerTaskStack;
	*pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

#ifdef DEBUG

struct StackMonitorContext{
	UBaseType_t stkMon_unusedStack;
	UBaseType_t clkCtrl_unusedStack;
	UBaseType_t supSys_unusedStack;
	UBaseType_t msg_unusedStack;
	UBaseType_t wsens_unusedStack;
	UBaseType_t voltMet_unusedStack;
	UBaseType_t gsm_unusedStack;
	UBaseType_t wless_unusedStack;
	UBaseType_t ui_unusedStack;
	UBaseType_t sound_unusedStack;
	UBaseType_t uartExt_unusedStack;
	UBaseType_t spiExt_unusedStack;
	UBaseType_t i2cExt_unusedStack;

	MessageBufferHandle_t msgIn, msgOut;
} stkMon_context;

void StackMonitor_mainTask(void * args){
	while(1){
		//update _unusedStack values
		if(taskHandleRegister.clkCtrl_handle){
			stkMon_context.clkCtrl_unusedStack=uxTaskGetStackHighWaterMark(taskHandleRegister.clkCtrl_handle);
		}
		if(taskHandleRegister.supSys_handle){
			stkMon_context. supSys_unusedStack=uxTaskGetStackHighWaterMark(taskHandleRegister.supSys_handle);
		}
		if(taskHandleRegister.msg_handle){
			stkMon_context. msg_unusedStack=uxTaskGetStackHighWaterMark(taskHandleRegister.msg_handle);
		}
		if(taskHandleRegister.wsens_handle){
			stkMon_context. wsens_unusedStack=uxTaskGetStackHighWaterMark(taskHandleRegister.wsens_handle);
		}
		if(taskHandleRegister.voltMet_handle){
			stkMon_context. voltMet_unusedStack=uxTaskGetStackHighWaterMark(taskHandleRegister.voltMet_handle);
		}
		if(taskHandleRegister.gsm_handle){
			stkMon_context. gsm_unusedStack=uxTaskGetStackHighWaterMark(taskHandleRegister.gsm_handle);
		}
		if(taskHandleRegister.wless_handle){
			stkMon_context. wless_unusedStack=uxTaskGetStackHighWaterMark(taskHandleRegister.wless_handle);
		}
		if(taskHandleRegister.ui_handle){
			stkMon_context. ui_unusedStack=uxTaskGetStackHighWaterMark(taskHandleRegister.ui_handle);
		}
		if(taskHandleRegister.sound_handle){
			stkMon_context. sound_unusedStack=uxTaskGetStackHighWaterMark(taskHandleRegister.sound_handle);
		}
		if(taskHandleRegister.uartExt_handle){
			stkMon_context. uartExt_unusedStack=uxTaskGetStackHighWaterMark(taskHandleRegister.uartExt_handle);
		}
		if(taskHandleRegister.spiExt_handle){
			stkMon_context. spiExt_unusedStack=uxTaskGetStackHighWaterMark(taskHandleRegister.spiExt_handle);
		}
		if(taskHandleRegister.i2cExt_handle){
			stkMon_context. i2cExt_unusedStack=uxTaskGetStackHighWaterMark(taskHandleRegister.i2cExt_handle);
		}
		//check own stack
		stkMon_context.stkMon_unusedStack=uxTaskGetStackHighWaterMark(taskHandleRegister.stkMon_handle);
		taskYIELD();
	}
}

TaskHandle_t StackMonitor_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	TaskHandle_t mainTask_handle;
	static const char mainTask_name[configMAX_TASK_NAME_LEN]="StackMonitor";
	static StaticTask_t mainTask_TCB;
	static StackType_t mainTask_stackBuffer[128]; //length=mainTask_stackDepth

	stkMon_context.msgIn=msgIn;
	stkMon_context.msgOut=msgOut;

	// Create task
	mainTask_handle=xTaskCreateStatic(StackMonitor_mainTask,
			mainTask_name,
			128, //mainTask_stackDepth
			(void *)0,
			1,
			mainTask_stackBuffer,
			&mainTask_TCB);

	return mainTask_handle;
}

#endif //DEBUG
