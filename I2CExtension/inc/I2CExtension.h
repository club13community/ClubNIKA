/*
 * I2CExtension.h
 *
 *  Created on: 1 трав. 2020 р.
 *      Author: MaxCm
 */

#ifndef _I2CEXTENSION_H_
#define _I2CEXTENSION_H_

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

TaskHandle_t I2CExtension_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut);

#ifdef __cplusplus
}
#endif

#endif /* _I2CEXTENSION_H_ */
