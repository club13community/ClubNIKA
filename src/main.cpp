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
#include "flash.h"
#include "UserInterface.h"
#include "ff.h"
#include "ff_flash_driver.h"
#include "sd_fs.h"
#include "stack_monitor.h"
#include "logging.h"
#include "drives.h"
#include "settings.h"
#include "periph_allocation.h"

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

static void create_app_starter();

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
	flash::init_periph();
	gsm::init_periph();
	sound_service::init_periph();
	user_interface::init_periph();
	wired_zones::init_periph();

	flash::init_disk_driver();

	MessageRouter_StartUpInit();

	ClockControl_Launch(getMessageBuffer_ClockControlDataIn(), getMessageBuffer_ClockControlDataOut());
	SupplySystem_Launch(getMessageBuffer_SupplySystemDataIn(), getMessageBuffer_SupplySystemDataOut());
	vmeter::start();
	user_interface::start();
	wired_zones::start();
	sd::start();
	rec::start();
	gsm::start();

	//wireless
	//File system
	//idle task
	//timer task
	create_test_task();
	create_app_starter();
#ifdef DEBUG
	start_stack_monitor();
#endif

	vTaskStartScheduler();
}

static void start_app(void * args) {
	f_mount(&flash_fs, "/flash", 1);
	load_settings();
	vTaskSuspend(NULL);
}

static void create_app_starter() {
	constexpr size_t stack_size = 192;
	static StaticTask_t task_ctrl;
	static StackType_t task_stack[stack_size];
	xTaskCreateStatic(start_app, "app. starter", stack_size, nullptr, APP_STARTER_PRIORITY, task_stack, &task_ctrl);
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
