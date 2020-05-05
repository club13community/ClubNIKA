/*
 * Message.c
 *
 *  Created on: 4 трав. 2020 р.
 *      Author: MaxCm
 */
#include "MessageChannelConfig.h"
#include "Message.h"

MsgReadResult MsgRead(MessageBufferHandle_t msgIn, uint32_t * msgControl, uint8_t * dataBuff, uint8_t * dataLength, uint8_t dataBuffLength){
	uint8_t msgBuff[MaxMsgLength], msgLen;
	uint8_t i, j;
	msgLen=xMessageBufferReceive(msgIn, msgBuff, MaxMsgLength, 0);
	if(0==msgLen){
		//no message, length of msgBuff is sufficient by design
		return MsgRead_NoMsg;
	} else {
		msgLen-=4; //no underflow by design
		if(dataBuffLength<msgLen){
			//dataBuff is too small
			return MsgRead_SmallBuff;
		} else {
			//save message
			*msgControl=*(uint32_t *)msgBuff;
			//save data
			for(i=0, j=4; i<msgLen; i++, j++){
				dataBuff[i]=msgBuff[j];
			}
			*dataLength=msgLen;
		}
	}
	return MsgRead_OK;
}

MsgSendResult MsgSend_Set12VChannelState(MessageBufferHandle_t msgOut, uint8_t state){
	uint8_t msgBuff[4+1];
#if MaxMsgLength_supSys_toService<5
#error "MaxMsgLength_supSys_toService is smaller, that 5"
#endif
	*(uint32_t *)msgBuff=MsgId_Set12VChannelState | (SupplySystemChannel<<MsgDestinationBase);
	msgBuff[4]=state;
	xMessageBufferSend(msgOut, msgBuff, 5, portMAX_DELAY);
	return MsgSend_OK;
}

MsgSendResult MsgSend_SetAlarmState(MessageBufferHandle_t msgOut, uint8_t state){
	uint8_t msgBuff[4+1];
#if MaxMsgLength_supSys_toService<5
#error "MaxMsgLength_supSys_toService is smaller, that 5"
#endif
	*(uint32_t *)msgBuff=MsgId_SetAlarmState | (SupplySystemChannel<<MsgDestinationBase);
	msgBuff[4]=state;
	xMessageBufferSend(msgOut, msgBuff, 5, portMAX_DELAY);
	return MsgSend_OK;
}

MsgSendResult MsgSend_SendViaUARTExtension(MessageBufferHandle_t msgOut, uint8_t * data, uint8_t dataLength){
	uint8_t msgBuff[MaxMsgLength];
	uint8_t i, j;
	if(dataLength+4>MaxMsgLength_uartExt_toService){
		//too many data
		return MsgSend_BigMsg;
	}
	*(uint32_t *)msgBuff=MsgId_SendViaUARTExtension | (UARTExtensionChannel<<MsgDestinationBase);
	//copy data
	for(i=0, j=4; i<dataLength; i++, j++){
		msgBuff[j]=data[i];
	}
	xMessageBufferSend(msgOut, msgBuff, dataLength+4, portMAX_DELAY);
	return MsgSend_OK;
}
