/*
 * GSMService.c
 *
 *  Created on: 3 квіт. 2020 р.
 *      Author: MaxCm
 */
#include "GSMService.h"
#include "IRQPriorityConfig.h"

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_dma.h"
#include <string.h>
#include "./sim900_periph_utils.h"
#include "rcc_utils.h"
#include "sim900_driver.h"

//Peripherals: UART3(ext. UART)
struct GSMServiceContext{
	TaskHandle_t mainTask_handle;
	MessageBufferHandle_t msgIn, msgOut;
} gsm_context;

void GSMService_initState(){

}

void gsm_service::init_periph() {
	sim900::config_peripherals();
}

void GSMService_mainTask(void *pvParameters){
	while(1){
		taskYIELD();
	}
}

TaskHandle_t GSMService_registerInOS(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	static const char mainTask_name[configMAX_TASK_NAME_LEN]="GSMService";
	static StaticTask_t mainTask_TCB;
	static StackType_t mainTask_stackBuffer[32]; //length=mainTask_stackDepth

	gsm_context.msgIn=msgIn;
	gsm_context.msgOut=msgOut;

	gsm_context.mainTask_handle=xTaskCreateStatic(GSMService_mainTask,
			mainTask_name,
			32, //mainTask_stackDepth
			(void *)0,
			1,
			mainTask_stackBuffer,
			&mainTask_TCB);

	return gsm_context.mainTask_handle;
}

static char get_product_cmd[20] = "ATI\r";
static char get_imei[10] = "AT+GSN\r";

static void emei_requested() {
	__NOP();
}

static void product_requested() {
	//sim900::send(get_imei, 7, emei_requested);
	__NOP();
}

static void turned_on(bool ok) {
	sim900::send(get_product_cmd, 4, product_requested);
}

TaskHandle_t GSMService_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	TaskHandle_t mainTask_handle;
	GSMService_initState();
	mainTask_handle=GSMService_registerInOS(msgIn, msgOut);
	return mainTask_handle;
}
