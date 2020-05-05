
#include "WiredSensorMonitor.h"

struct WiredSensorMonitorContext{
	TaskHandle_t mainTask_handle;
	MessageBufferHandle_t msgIn, msgOut;
} wsens_context;

void WiredSensorMonitor_mainTask(void *pvParameters){
	while(1){
		taskYIELD();
	}
}

void WiredSensorMonitor_initState(){

}

void WiredSensorMonitor_configPeripherals(){

}

TaskHandle_t WiredSensorMonitor_registerInOS(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	// Create tasks
	static const char mainTask_name[configMAX_TASK_NAME_LEN]="WSensorMonitor";
	static StaticTask_t mainTask_TCB;
	static StackType_t mainTask_stackBuffer[32]; //length=mainTask_stackDepth

	wsens_context.msgIn=msgIn;
	wsens_context.msgOut=msgOut;

	wsens_context.mainTask_handle=xTaskCreateStatic(WiredSensorMonitor_mainTask,
			mainTask_name,
			32, //mainTask_stackDepth
			(void *)0,
			1,
			mainTask_stackBuffer,
			&mainTask_TCB);

	return wsens_context.mainTask_handle;

}

TaskHandle_t WiredSensorMonitor_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	TaskHandle_t mainTask_handle;
	WiredSensorMonitor_initState();
	WiredSensorMonitor_configPeripherals();
	mainTask_handle=WiredSensorMonitor_registerInOS(msgIn, msgOut);
	return mainTask_handle;
}
