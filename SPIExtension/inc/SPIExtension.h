/*
 * SPIExtention.h
 *
 *  Created on: 1 ����. 2020 �.
 *      Author: MaxCm
 */

#ifndef _SPIEXTENSION_H_
#define _SPIEXTENSION_H_

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"

void SPIExtension_CommFinished_IH();

TaskHandle_t SPIExtension_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut);


#endif /* _SPIEXTENSION_H_ */
