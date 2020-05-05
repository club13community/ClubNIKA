/*
 * I2CExtension.c
 *
 *  Created on: 1 трав. 2020 р.
 *      Author: MaxCm
 */
#include "I2CExtension.h"

struct I2CExtensionContext{
	TaskHandle_t mainTask_handle;
	MessageBufferHandle_t msgIn, msgOut;
} i2cExt_context;

void I2CExtension_initState(){

}

void I2CExtension_configPeripherals(){

}

void I2CExtension_mainTask(void *pvParameters){
	while(1){
		taskYIELD();
	}
}

TaskHandle_t I2CExtension_registerInOS(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	//Save message channels
	i2cExt_context.msgIn=msgIn;
	i2cExt_context.msgOut=msgOut;

	//Create tasks
	static const char mainTask_name[configMAX_TASK_NAME_LEN]="I2CExtension";
	static StaticTask_t mainTask_TCB;
	static StackType_t mainTask_stackBuffer[32]; //length=mainTask_stackDepth

	i2cExt_context.mainTask_handle=xTaskCreateStatic(I2CExtension_mainTask,
			mainTask_name,
			32, //mainTask_stackDepth
			(void *)0,
			1,
			mainTask_stackBuffer,
			&mainTask_TCB);

	return i2cExt_context.mainTask_handle;
}

TaskHandle_t I2CExtension_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	TaskHandle_t mainTask_handle;
	I2CExtension_initState();
	I2CExtension_configPeripherals();
	mainTask_handle=I2CExtension_registerInOS(msgIn, msgOut);
	return mainTask_handle;
}
