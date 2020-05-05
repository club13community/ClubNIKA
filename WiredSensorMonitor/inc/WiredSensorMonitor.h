/*
 * WiredSensorMonitor.h
 *
 *  Created on: 3 квіт. 2020 р.
 *      Author: MaxCm
 */

#ifndef _WIREDSENSORMONITOR_H_
#define _WIREDSENSORMONITOR_H_

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"

TaskHandle_t WiredSensorMonitor_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut);

#endif /* INC_WIREDSENSORMONITOR_H_ */
