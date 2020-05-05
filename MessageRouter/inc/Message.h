/*
 * messageList.h
 *
 *  Created on: 29 квіт. 2020 р.
 *      Author: MaxCm
 */

#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <stdint.h>
#include "FreeRTOS.h"
#include "message_buffer.h"

#define MaxMsgLength	16

//structure of message
//[31:27] - message source
//[26:22] - message destination
//[21:11] - reserved
//[10:8] - message type
//[7:0] - message code
//rest bytes

#define MsgSourceBase	27
#define MsgSourceMask	((uint32_t)(0x1F<<MsgSourceBase))

#define MsgDestinationBase	22
#define MsgDestinationMask	((uint32_t)(0x1F<<MsgDestinationBase))

#define MsgTypeBase				8
#define MsgTypeMask				((uint32_t)(0x7<<MsgTypeBase))
#define MsgType_Request			((uint32_t)(1<<MsgTypeBase)) //request to get value
#define MsgType_Answer			((uint32_t)(2<<MsgTypeBase)) //answer on request
#define MsgType_Action			((uint32_t)(3<<MsgTypeBase)) //do something
#define MsgType_Info			((uint32_t)(4<<MsgTypeBase)) //notify about something
#define MsgType_NoSupport		((uint32_t)(5<<MsgTypeBase))

#define MsgCodeBase		0
#define MsgCodeMask		((uint32_t)(0xFF<<MsgCodeBase))

#define MsgIdMask	(MsgTypeMask | MsgCodeMask)

#if defined(_SUPPLYSYSTEM_H_) || defined(_MESSAGECHANNELCONFIG_H_)
//To SupplySystem
#define MsgId_Set12VChannelState	(MsgType_Action | (uint32_t)(1<<MsgCodeBase))
#define MsgId_SetAlarmState			(MsgType_Action | (uint32_t)(2<<MsgCodeBase))
#endif

#if defined(_SUPPLYSYSTEM_H_) || defined(_MESSAGECHANNELCONFIG_H_)
//To UARTExtension
#define MsgId_SendViaUARTExtension (MsgType_Action | (uint32_t)(3<<MsgCodeBase))
#endif

#if defined (_VOLTAGEMETER_H_) || defined(_MESSAGECHANNELCONFIG_H_)
//To VoltageMeter
#define MsgId_GetWiredSensorSourceCurrent	(MsgType_Request | (uint32_t)(4<<MsgCodeBase))
#define MsgId_GetWiredSensorSinkCurrent		(MsgType_Request | (uint32_t)(5<<MsgCodeBase))
#define MsgId_GetBatteryVoltage				(MsgType_Request | (uint32_t)(6<<MsgCodeBase))

//From VoltageMeter
#define MsgId_WiredSensorSourceCurrent		(MsgId_GetWiredSensorSourceCurrent & ~MsgTypeMask | MsgType_Answer)
#define MsgId_WiredSensorSinkCurrent		(MsgId_GetWiredSensorSinkCurrent & ~MsgTypeMask | MsgType_Answer)
#define MsgId_BatteryVoltage				(MsgId_GetBatteryVoltage & ~MsgTypeMask | MsgType_Answer)
#endif

typedef uint8_t MsgReadResult;
#define MsgRead_OK			((MsgReadResult)0)
#define MsgRead_NoMsg		((MsgReadResult)1)
#define MsgRead_SmallBuff	((MsgReadResult)2)

MsgReadResult MsgRead(MessageBufferHandle_t msgIn, uint32_t * msg, uint8_t * dataBuff, uint8_t * dataLength, uint8_t dataBuffLength);

typedef uint8_t MsgSendResult;
#define MsgSend_OK		((MsgSendResult)0)
#define MsgSend_BigMsg	((MsgSendResult)1)
#define MsgSend_Fail	((MsgSendResult)1)

MsgSendResult MsgSend_Set12VChannelState(MessageBufferHandle_t msgOut, uint8_t state);
MsgSendResult MsgSend_SetAlarmState(MessageBufferHandle_t msgOut, uint8_t state);

MsgSendResult MsgSend_SendViaUARTExtension(MessageBufferHandle_t msgOut, uint8_t * data, uint8_t dataLength);

#endif /* _MESSAGE_H_ */
