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
#include "sim900_periph_utils.h"
#include "rcc_utils.h"
#include "sim900_driver.h"

//Peripherals: UART3(ext. UART)
struct GSMServiceContext{
	TaskHandle_t mainTask_handle;
	MessageBufferHandle_t msgIn, msgOut;
} gsm_context;

void GSMService_initState(){

}

void GSMService_configPeripherals() {
	sim900::config_peripherals();

	/*GPIO_InitTypeDef pinInitStruct;
	// VBAT discharge port
	enable_periph_clock(VBAT_DISCHARGE_PORT);
	sim900::open_vbat(); // do not discharge
	pinInitStruct.GPIO_Pin = VBAT_DISCHARGE_PIN;
	pinInitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	pinInitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(VBAT_DISCHARGE_PORT, &pinInitStruct);

	// VBAT switch
	enable_periph_clock(VBAT_SW_PORT);
	sim900::enable_vbat(); // on VBAT
	pinInitStruct.GPIO_Pin = VBAT_SW_PIN;
	pinInitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	pinInitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(VBAT_SW_PORT, &pinInitStruct);*/


}

/*
 * Interrupt handler
 */
static char rx_buf[128], tx_buf[128];
uint8_t rx_ind = 0, tx_ind = 0, tx_len = 0;

#define as_byte(x) (*(uint8_t *)&(x))

uint8_t is_write_done() {
	return tx_len == 0;
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
	GSMService_configPeripherals();
	mainTask_handle=GSMService_registerInOS(msgIn, msgOut);
	sim900::turn_on(turned_on);
	return mainTask_handle;
}
