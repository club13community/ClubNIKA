/*
 * SoundService.h
 *
 *  Created on: 3 квіт. 2020 р.
 *      Author: MaxCm
 */
#include "SoundService.h"
#include "speaker.h"

#include "ClockControl.h"

struct SoundServiceContext{
	TaskHandle_t mainTask_handle;
	MessageBufferHandle_t msgIn, msgOut;

} sound_context;

volatile uint32_t acquisitionBuffer[128];
volatile uint8_t playBuffer1[128];
volatile uint8_t playBuffer2[128];
volatile bool buffer1 = true;

const uint8_t p2p = 200;
const uint8_t points = 96;

static speaker::Data getData() {
	if (buffer1) {
		buffer1 = false;
		return {.samples = const_cast<uint8_t *>(playBuffer1), .samplesLength = points/2};
	} else {
		buffer1 = true;
		return {.samples = const_cast<uint8_t *>(playBuffer2), .samplesLength = points/2};
	}
}

static void end() {

}


void SoundService_initState() {

}

void sound_service::init_periph(){
	speaker::initPeripherals();
}

void SoundService_mainTask(void *pvParameters){
	while(1){
		taskYIELD();
	}
}

TaskHandle_t SoundService_registerInOS(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	sound_context.msgIn=msgIn;
	sound_context.msgOut=msgOut;

	static const char mainTask_name[configMAX_TASK_NAME_LEN]="SoundService";
	static StaticTask_t mainTask_TCB;
	static StackType_t mainTask_stackBuffer[32]; //length=mainTask_stackDepth

	// Create tasks
	sound_context.mainTask_handle=xTaskCreateStatic(SoundService_mainTask,
			mainTask_name,
			32, //mainTask_stackDepth
			(void *)0,
			1,
			mainTask_stackBuffer,
			&mainTask_TCB);

	return sound_context.mainTask_handle;
}

#define SAMPLING_48K	21U

TaskHandle_t SoundService_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	TaskHandle_t mainTask_handle;
	SoundService_initState();

	mainTask_handle=SoundService_registerInOS(msgIn, msgOut);

	uint8_t real_step = p2p / (points / 2);
	uint8_t real_p2p = real_step * (points / 2);
	uint8_t low = (256 - real_p2p) / 2;
	uint8_t high = low + real_p2p;

	for (uint8_t i = 0; i < points / 2; i++) {
		playBuffer1[i] = i * real_step + low;
		playBuffer2[i] = high - real_step * i;
	}
	speaker::unmute();
	speaker::selectDac();
	speaker::playOnDac(SAMPLING_48K, getData, end);

	return mainTask_handle;
}

