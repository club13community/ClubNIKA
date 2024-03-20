/*
 * MessageRouter.h
 *
 *  Created on: 29 квіт. 2020 р.
 *      Author: MaxCm
 */

#ifndef _MESSAGEROUTER_H_
#define _MESSAGEROUTER_H_

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

void MessageRouter_StartUpInit();

TaskHandle_t MessageRouter_Launch();

MessageBufferHandle_t getMessageBuffer_SupplySystemDataOut();

MessageBufferHandle_t getMessageBuffer_SupplySystemDataIn();

MessageBufferHandle_t getMessageBuffer_ClockControlDataOut();

MessageBufferHandle_t getMessageBuffer_ClockControlDataIn();

MessageBufferHandle_t getMessageBuffer_WiredSensorMonitorDataOut();

MessageBufferHandle_t getMessageBuffer_WiredSensorMonitorDataIn();

MessageBufferHandle_t getMessageBuffer_VoltageMeterDataOut();

MessageBufferHandle_t getMessageBuffer_VoltageMeterDataIn();

MessageBufferHandle_t getMessageBuffer_GSMServiceDataOut();

MessageBufferHandle_t getMessageBuffer_GSMServiceDataIn();

MessageBufferHandle_t getMessageBuffer_WirelessInterfaceDataOut();

MessageBufferHandle_t getMessageBuffer_WirelessInterfaceDataIn();

MessageBufferHandle_t getMessageBuffer_UserInterfaceDataOut();

MessageBufferHandle_t getMessageBuffer_UserInterfaceDataIn();

MessageBufferHandle_t getMessageBuffer_SoundServiceDataOut();

MessageBufferHandle_t getMessageBuffer_SoundServiceDataIn();

MessageBufferHandle_t getMessageBuffer_UARTExtensionDataOut();

MessageBufferHandle_t getMessageBuffer_UARTExtensionDataIn();

MessageBufferHandle_t getMessageBuffer_SPIExtensionDataOut();

MessageBufferHandle_t getMessageBuffer_SPIExtensionDataIn();

MessageBufferHandle_t getMessageBuffer_I2CExtensionDataOut();

MessageBufferHandle_t getMessageBuffer_I2CExtensionDataIn();

#ifdef DEBUG

MessageBufferHandle_t getMessageBuffer_StackMonitorDataOut();

MessageBufferHandle_t getMessageBuffer_StackMonitorDataIn();

#endif

#ifdef __cplusplus
}
#endif

#endif /* INC_MESSAGEROUTER_H_ */
