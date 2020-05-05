/*
 * ClockControl.h
 *
 *  Created on: 24 ���. 2020 �.
 *      Author: MaxCm
 */

#ifndef _CLOCKCONTROL_H_
#define _CLOCKCONTROL_H_
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"

void ClockControl_StartUpInit();
TaskHandle_t ClockControl_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut);
int32_t getPeripheralClockFrequencyKHz(uint32_t periphBaseAddr);

#endif /* INC_CLOCKCONTROL_H_ */
