/*
 * VoltageMeter.h
 *
 *  Created on: 4 трав. 2020 р.
 *      Author: MaxCm
 */

#ifndef _VOLTAGEMETER_H_
#define _VOLTAGEMETER_H_

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

void VoltageMeter_EndOfConversion_IH();

TaskHandle_t VoltageMeter_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut);

#ifdef __cplusplus
}
#endif

#endif /* _VOLTAGEMETER_H_ */
