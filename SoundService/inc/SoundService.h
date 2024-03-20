/*
 * SoundService.h
 *
 *  Created on: 3 ���. 2020 �.
 *      Author: MaxCm
 */

#ifndef _SOUNDSERVICE_H_
#define _SOUNDSERVICE_H_

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

TaskHandle_t SoundService_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut);

#ifdef __cplusplus
}
#endif

#endif /* _SOUNDSERVICE_H_ */
