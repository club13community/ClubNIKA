/*
 * UARTExtension.h
 *
 *  Created on: 1 трав. 2020 р.
 *      Author: MaxCm
 */

#ifndef _UARTEXTENSION_H_
#define _UARTEXTENSION_H_

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

void UARTExtension_Rx_IH();

void UARTExtension_TxComplete_IH();

TaskHandle_t UARTExtension_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut);

#ifdef __cplusplus
}
#endif

#endif /* _UARTEXTENSION_H_ */
