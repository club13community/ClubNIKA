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
#include "ff.h"
#include "ff_flash_driver.h"
#include "sd_driver.h"
#include "sd_fs.h"
#include "stack_monitor.h"
#include "logging.h"
#include "../Logging/Buffer.h"
#include "drives.h"

static TaskHandle_t test_task;
static StaticTask_t test_task_ctrl;
static StackType_t test_task_stack[1024];
static FIL tst_file;


static void do_test_task(void * args) {
	while(true);
}

static void create_test_task() {
	test_task = xTaskCreateStatic(do_test_task, "test task", 1024, nullptr, 1U, test_task_stack, &test_task_ctrl);
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

	flash::init();
	gsm_service::init_periph();
	sound_service::init_periph();
	user_interface::init_periph();
	wired_zones::init_periph();

	MessageRouter_StartUpInit();

	ClockControl_Launch(getMessageBuffer_ClockControlDataIn(), getMessageBuffer_ClockControlDataOut());
	SupplySystem_Launch(getMessageBuffer_SupplySystemDataIn(), getMessageBuffer_SupplySystemDataOut());
	vmeter::start();
	user_interface::start();
	wired_zones::start();
	sd::start();
	rec::start();
	//taskHandleRegister.wsens_handle=WiredSensorMonitor_Launch(getMessageBuffer_WiredSensorMonitorDataIn(), getMessageBuffer_WiredSensorMonitorDataOut());
	//taskHandleRegister.voltMet_handle=VoltageMeter_Launch(getMessageBuffer_VoltageMeterDataIn(), getMessageBuffer_VoltageMeterDataOut());
	//taskHandleRegister.wless_handle=WirelessInterface_Launch(getMessageBuffer_WirelessInterfaceDataIn(), getMessageBuffer_WirelessInterfaceDataOut());
	//taskHandleRegister.ui_handle=UserInterface_Launch(getMessageBuffer_UserInterfaceDataIn(), getMessageBuffer_UserInterfaceDataOut());
	//taskHandleRegister.uartExt_handle=UARTExtension_Launch(getMessageBuffer_UARTExtensionDataIn(), getMessageBuffer_UARTExtensionDataOut());
	//taskHandleRegister.spiExt_handle=SPIExtension_Launch(getMessageBuffer_SPIExtensionDataIn(), getMessageBuffer_SPIExtensionDataOut());
	//taskHandleRegister.i2cExt_handle=I2CExtension_Launch(getMessageBuffer_I2CExtensionDataIn(), getMessageBuffer_I2CExtensionDataOut());

	//taskHandleRegister.msg_handle=MessageRouter_Launch();

	//wireless
	//File system
	//idle task
	//timer task
	create_test_task();
#ifdef DEBUG
	start_stack_monitor();
#endif

	vTaskStartScheduler();
}


extern "C" void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    size_t *pulIdleTaskStackSize ){
	static StaticTask_t xIdleTaskTCB;
	static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

	*ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
	*ppxIdleTaskStackBuffer = uxIdleTaskStack;
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

extern "C" void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t **ppxTimerTaskStackBuffer,
                                     size_t *pulTimerTaskStackSize ){
	static StaticTask_t xTimerTaskTCB;
	static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

	*ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
	*ppxTimerTaskStackBuffer = uxTimerTaskStack;
	*pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
