/*
 * SupplySystem.h
 *
 *  Created on: 30 ���. 2020 �.
 *      Author: MaxCm
 */

#ifndef _SUPPLYSYSTEM_H_
#define _SUPPLYSYSTEM_H_

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const char SupplySystem_mainTaskName[];

void SupplySystem_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut);
void SupplySystem_12VChannelsOvercurrent_IH();
void SupplySystem_12VChannelsOvercurrent_Timer_IH();

#ifdef __cplusplus
}
#endif

#endif /* INC_SUPPLYSYSTEM_H_ */
