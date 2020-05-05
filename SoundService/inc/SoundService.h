/*
 * SoundService.h
 *
 *  Created on: 3 квіт. 2020 р.
 *      Author: MaxCm
 */

#ifndef _SOUNDSERVICE_H_
#define _SOUNDSERVICE_H_

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"

TaskHandle_t SoundService_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut);

#endif /* _SOUNDSERVICE_H_ */
