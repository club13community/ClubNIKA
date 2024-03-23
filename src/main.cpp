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
#include "WiredSensorMonitor.h"
#include "VoltageMeter.h"
#include "GSMService.h"
#include "WirelessInterface.h"
#include "UserInterface.h"
#include "SoundService.h"
#include "UARTExtension.h"
#include "SPIExtension.h"
#include "I2CExtension.h"
#include "Message.h"

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

int main(void)
{
	SystemInit();
	ClockControl_StartUpInit();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	timing::configPeripherals();
	timing::config_fine_timer();

	MessageRouter_StartUpInit();
	clearTaskHandleRegister();

	taskHandleRegister.clkCtrl_handle=ClockControl_Launch(getMessageBuffer_ClockControlDataIn(), getMessageBuffer_ClockControlDataOut());
	taskHandleRegister.supSys_handle=SupplySystem_Launch(getMessageBuffer_SupplySystemDataIn(), getMessageBuffer_SupplySystemDataOut());
	taskHandleRegister.msg_handle=MessageRouter_Launch();
	taskHandleRegister.wsens_handle=WiredSensorMonitor_Launch(getMessageBuffer_WiredSensorMonitorDataIn(), getMessageBuffer_WiredSensorMonitorDataOut());
	taskHandleRegister.voltMet_handle=VoltageMeter_Launch(getMessageBuffer_VoltageMeterDataIn(), getMessageBuffer_VoltageMeterDataOut());
	taskHandleRegister.gsm_handle=GSMService_Launch(getMessageBuffer_GSMServiceDataIn(), getMessageBuffer_GSMServiceDataOut());
	taskHandleRegister.wless_handle=WirelessInterface_Launch(getMessageBuffer_WirelessInterfaceDataIn(), getMessageBuffer_WirelessInterfaceDataOut());
	taskHandleRegister.ui_handle=UserInterface_Launch(getMessageBuffer_UserInterfaceDataIn(), getMessageBuffer_UserInterfaceDataOut());
	taskHandleRegister.sound_handle=SoundService_Launch(getMessageBuffer_SoundServiceDataIn(), getMessageBuffer_SoundServiceDataOut());
	taskHandleRegister.uartExt_handle=UARTExtension_Launch(getMessageBuffer_UARTExtensionDataIn(), getMessageBuffer_UARTExtensionDataOut());
	taskHandleRegister.spiExt_handle=SPIExtension_Launch(getMessageBuffer_SPIExtensionDataIn(), getMessageBuffer_SPIExtensionDataOut());
	taskHandleRegister.i2cExt_handle=I2CExtension_Launch(getMessageBuffer_I2CExtensionDataIn(), getMessageBuffer_I2CExtensionDataOut());

	//wireless
	//File system
	//idle task
	//timer task

#ifdef DEBUG
	taskHandleRegister.stkMon_handle=StackMonitor_Launch(getMessageBuffer_StackMonitorDataIn(), getMessageBuffer_StackMonitorDataOut());
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
