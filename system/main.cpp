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
#include "FreeRTOS.h"
#include "task.h"
#include "timing.h"
#include "ClockControl.h"
#include "SupplySystem.h"
#include "wired_zones.h"
#include "WirelessInterface.h"
#include "voltage_meter.h"
#include "GSMService.h"
#include "UserInterface.h"
#include "SoundService.h"
#include "flash.h"
#include "UserInterface.h"
#include "ff.h"
#include "ff_utils.h"
#include "ff_flash_driver.h"
#include "sd_fs.h"
#include "sd_driver.h"
#include "stack_monitor.h"
#include "logging.h"
#include "rtc.h"
#include "drives.h"
#include "settings.h"
#include "periph_allocation.h"
#include "alarm.h"
#include <string.h>

static void create_app_starter();

extern "C" int main(void)
{
	SystemInit();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	clocks::init();
	supply::init();

	timing::config_coarse_timer();
	timing::config_fine_timer();
	vmeter::init_periph();
	flash::init_periph();
	gsm::init_periph();
	user_interface::init_periph();
	wired_zones::init_periph();
	wireless::init_periph();

	flash::init_disk_driver();

	init_settings();
	rtc::start();
	vmeter::start();
	user_interface::start();
	wired_zones::start();
	sd::start();
	rec::start();
	gsm::start();
	player::start();
	alarm::start();

	create_app_starter();
#ifdef DEBUG
	start_stack_monitor();
#endif

	vTaskStartScheduler();
}

#define GREETING_WAV "/flash/greeting.wav"

static void start_app(void * args) {
	using rec::log, rec::s;
	FRESULT mount_res = f_mount(&flash_fs, "/flash", 1);
	if (mount_res != FR_OK) {
		log("Failed to mount flash. Error: {0}", {s(mount_res)});
	}
	bool setting_res = load_settings();
	if (!setting_res) {
		log("Failed to load settings. Use default.");
	}
	player::play_via_speaker(GREETING_WAV);
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

#ifdef DEBUG
static FIL tmp1, tmp2;
static void init_flash() {
	// NOTE: takes a lot of time
	bool settings_res = save_default_settings();
	FRESULT greeting_res = copy_from_sd_to_flash(GREETING_WAV, &tmp1, &tmp2);
	FRESULT alarm_res = alarm::copy_wav_to_flash(&tmp1, &tmp2);
	__NOP();
}
#endif