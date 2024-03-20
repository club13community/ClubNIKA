/*
 * UserInterface.h
 *
 *  Created on: 3 квіт. 2020 р.
 *      Author: MaxCm
 */

#ifndef _USERINTERFACE_H_
#define _USERINTERFACE_H_

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

TaskHandle_t UserInterface_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut);

void UserInterface_Timer_IH();

#ifdef __cplusplus
}
#endif

#endif /* _USERINTERFACE_H_ */
