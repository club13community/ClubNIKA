/*
 * MessageRouter.c
 *
 *  Created on: 29 квіт. 2020 р.
 *      Author: MaxCm
 */
#define _MESSAGE_SUPPORT_
#include "MessageChannelConfig.h"
#include "MessageRouter.h"
#include "Message.h"


#define MSG_BUFFER_LENGTH	20

struct MessageChannelForService {
MessageBufferHandle_t fromServise_handle;
MessageBufferHandle_t toServise_handle;
} messageChannels[NumberOfChannels];


MessageBufferHandle_t getMessageBuffer_SupplySystemDataOut(){return messageChannels[SupplySystemChannel].fromServise_handle;}
MessageBufferHandle_t getMessageBuffer_SupplySystemDataIn(){return messageChannels[SupplySystemChannel].toServise_handle;}

MessageBufferHandle_t getMessageBuffer_ClockControlDataOut(){return messageChannels[ClockControlChannel].fromServise_handle;}
MessageBufferHandle_t getMessageBuffer_ClockControlDataIn(){return messageChannels[ClockControlChannel].toServise_handle;}

MessageBufferHandle_t getMessageBuffer_WiredSensorMonitorDataOut(){return messageChannels[WiredSensorMonitorChannel].fromServise_handle;}
MessageBufferHandle_t getMessageBuffer_WiredSensorMonitorDataIn(){return messageChannels[WiredSensorMonitorChannel].toServise_handle;}

MessageBufferHandle_t getMessageBuffer_VoltageMeterDataOut(){return messageChannels[VoltageMeterChannel].fromServise_handle;}
MessageBufferHandle_t getMessageBuffer_VoltageMeterDataIn(){return messageChannels[VoltageMeterChannel].toServise_handle;}

MessageBufferHandle_t getMessageBuffer_GSMServiceDataOut(){return messageChannels[GSMServiceChannel].fromServise_handle;}
MessageBufferHandle_t getMessageBuffer_GSMServiceDataIn(){return messageChannels[GSMServiceChannel].toServise_handle;}

MessageBufferHandle_t getMessageBuffer_WirelessInterfaceDataOut(){return messageChannels[WirelessInterfaceChannel].fromServise_handle;}
MessageBufferHandle_t getMessageBuffer_WirelessInterfaceDataIn(){return messageChannels[WirelessInterfaceChannel].toServise_handle;}

MessageBufferHandle_t getMessageBuffer_UserInterfaceDataOut(){return messageChannels[UserInterfaceChannel].fromServise_handle;}
MessageBufferHandle_t getMessageBuffer_UserInterfaceDataIn(){return messageChannels[UserInterfaceChannel].toServise_handle;}

MessageBufferHandle_t getMessageBuffer_SoundServiceDataOut(){return messageChannels[SoundServiceChannel].fromServise_handle;}
MessageBufferHandle_t getMessageBuffer_SoundServiceDataIn(){return messageChannels[SoundServiceChannel].toServise_handle;}

MessageBufferHandle_t getMessageBuffer_UARTExtensionDataOut(){return messageChannels[UARTExtensionChannel].fromServise_handle;}
MessageBufferHandle_t getMessageBuffer_UARTExtensionDataIn(){return messageChannels[UARTExtensionChannel].toServise_handle;}

MessageBufferHandle_t getMessageBuffer_SPIExtensionDataOut(){return messageChannels[SPIExtensionChannel].fromServise_handle;}
MessageBufferHandle_t getMessageBuffer_SPIExtensionDataIn(){return messageChannels[SPIExtensionChannel].toServise_handle;}

MessageBufferHandle_t getMessageBuffer_I2CExtensionDataOut(){return messageChannels[I2CExtensionChannel].fromServise_handle;}
MessageBufferHandle_t getMessageBuffer_I2CExtensionDataIn(){return messageChannels[I2CExtensionChannel].toServise_handle;}

#ifdef DEBUG
MessageBufferHandle_t getMessageBuffer_StackMonitorDataOut(){return messageChannels[StackMonitorChannel].fromServise_handle;}
MessageBufferHandle_t getMessageBuffer_StackMonitorDataIn(){return messageChannels[StackMonitorChannel].toServise_handle;}
#endif


//create message buffers for services
void MessageRouter_StartUpInit(){
	//SupplySystem
	static uint8_t supSys_fromService_storageArea[MaxMsgLength_supSys_fromService+sizeof(size_t)];
	static StaticMessageBuffer_t supSys_fromService_dataStruct;
	messageChannels[SupplySystemChannel].fromServise_handle=xMessageBufferCreateStatic(MaxMsgLength_supSys_fromService+sizeof(size_t),
			supSys_fromService_storageArea,	&supSys_fromService_dataStruct);

	static uint8_t supSys_toService_storageArea[MaxMsgLength_supSys_toService+sizeof(size_t)];
	static StaticMessageBuffer_t supSys_toService_dataStruct;
	messageChannels[SupplySystemChannel].toServise_handle=xMessageBufferCreateStatic(MaxMsgLength_supSys_toService+sizeof(size_t),
			supSys_toService_storageArea, &supSys_toService_dataStruct);
	//ClockControl
	static uint8_t clkCtrl_fromService_storageArea[MSG_BUFFER_LENGTH];
	static StaticMessageBuffer_t clkCtrl_fromService_dataStruct;
	messageChannels[ClockControlChannel].fromServise_handle=xMessageBufferCreateStatic(MSG_BUFFER_LENGTH, clkCtrl_fromService_storageArea,
			&clkCtrl_fromService_dataStruct);

	static uint8_t clkCtrl_toService_storageArea[MSG_BUFFER_LENGTH];
	static StaticMessageBuffer_t clkCtrl_toService_dataStruct;
	messageChannels[ClockControlChannel].toServise_handle=xMessageBufferCreateStatic(MSG_BUFFER_LENGTH, clkCtrl_toService_storageArea,
			&clkCtrl_toService_dataStruct);
	//WiredSensorMonitor
	static uint8_t wsens_fromService_storageArea[MSG_BUFFER_LENGTH];
	static StaticMessageBuffer_t wsens_fromService_dataStruct;
	messageChannels[WiredSensorMonitorChannel].fromServise_handle=xMessageBufferCreateStatic(MSG_BUFFER_LENGTH, wsens_fromService_storageArea,
			&wsens_fromService_dataStruct);

	static uint8_t wsens_toService_storageArea[MSG_BUFFER_LENGTH];
	static StaticMessageBuffer_t wsens_toService_dataStruct;
	messageChannels[WiredSensorMonitorChannel].toServise_handle=xMessageBufferCreateStatic(MSG_BUFFER_LENGTH, wsens_toService_storageArea,
			&wsens_toService_dataStruct);
	//VoltageMeter
	static uint8_t voltMet_fromService_storageArea[MaxMsgLength_voltMet_fromService+sizeof(size_t)];
	static StaticMessageBuffer_t voltMet_fromService_dataStruct;
	messageChannels[VoltageMeterChannel].fromServise_handle=xMessageBufferCreateStatic(MaxMsgLength_voltMet_fromService+sizeof(size_t),
			voltMet_fromService_storageArea, &voltMet_fromService_dataStruct);

	static uint8_t voltMet_toService_storageArea[MaxMsgLength_voltMet_toService+sizeof(size_t)];
	static StaticMessageBuffer_t voltMet_toService_dataStruct;
	messageChannels[VoltageMeterChannel].toServise_handle=xMessageBufferCreateStatic(MaxMsgLength_voltMet_toService+sizeof(size_t),
			voltMet_toService_storageArea, &voltMet_toService_dataStruct);
	//GSMService
	static uint8_t gsm_fromService_storageArea[MSG_BUFFER_LENGTH];
	static StaticMessageBuffer_t gsm_fromService_dataStruct;
	messageChannels[GSMServiceChannel].fromServise_handle=xMessageBufferCreateStatic(MSG_BUFFER_LENGTH, gsm_fromService_storageArea,
			&gsm_fromService_dataStruct);

	static uint8_t gsm_toService_storageArea[MSG_BUFFER_LENGTH];
	static StaticMessageBuffer_t gsm_toService_dataStruct;
	messageChannels[GSMServiceChannel].toServise_handle=xMessageBufferCreateStatic(MSG_BUFFER_LENGTH, gsm_toService_storageArea,
			&gsm_toService_dataStruct);
	//WirelessInterface
	static uint8_t wless_fromService_storageArea[MSG_BUFFER_LENGTH];
	static StaticMessageBuffer_t wless_fromService_dataStruct;
	messageChannels[WirelessInterfaceChannel].fromServise_handle=xMessageBufferCreateStatic(MSG_BUFFER_LENGTH,
			wless_fromService_storageArea, &wless_fromService_dataStruct);

	static uint8_t wless_toService_storageArea[MSG_BUFFER_LENGTH];
	static StaticMessageBuffer_t wless_toService_dataStruct;
	messageChannels[WirelessInterfaceChannel].toServise_handle=xMessageBufferCreateStatic(MSG_BUFFER_LENGTH,
			wless_toService_storageArea, &wless_toService_dataStruct);
	//UserInterface
	static uint8_t ui_fromService_storageArea[MSG_BUFFER_LENGTH];
	static StaticMessageBuffer_t ui_fromService_dataStruct;
	messageChannels[UserInterfaceChannel].fromServise_handle=xMessageBufferCreateStatic(MSG_BUFFER_LENGTH, ui_fromService_storageArea,
			&ui_fromService_dataStruct);

	static uint8_t ui_toService_storageArea[MSG_BUFFER_LENGTH];
	static StaticMessageBuffer_t ui_toService_dataStruct;
	messageChannels[UserInterfaceChannel].toServise_handle=xMessageBufferCreateStatic(MSG_BUFFER_LENGTH, ui_toService_storageArea,
			&ui_toService_dataStruct);
	//SoundService
	static uint8_t sound_fromService_storageArea[MSG_BUFFER_LENGTH];
	static StaticMessageBuffer_t sound_fromService_dataStruct;
	messageChannels[SoundServiceChannel].fromServise_handle=xMessageBufferCreateStatic(MSG_BUFFER_LENGTH, sound_fromService_storageArea,
			&sound_fromService_dataStruct);

	static uint8_t sound_toService_storageArea[MSG_BUFFER_LENGTH];
	static StaticMessageBuffer_t sound_toService_dataStruct;
	messageChannels[SoundServiceChannel].toServise_handle=xMessageBufferCreateStatic(MSG_BUFFER_LENGTH, sound_toService_storageArea,
			&sound_toService_dataStruct);
	//UARTExtension
	static uint8_t uartExt_fromService_storageArea[MaxMsgLength_uartExt_fromService+sizeof(size_t)];
	static StaticMessageBuffer_t uartExt_fromService_dataStruct;
	messageChannels[UARTExtensionChannel].fromServise_handle=xMessageBufferCreateStatic(MaxMsgLength_uartExt_fromService+sizeof(size_t),
			uartExt_fromService_storageArea, &uartExt_fromService_dataStruct);

	static uint8_t uartExt_toService_storageArea[MaxMsgLength_uartExt_toService+sizeof(size_t)];
	static StaticMessageBuffer_t uartExt_toService_dataStruct;
	messageChannels[UARTExtensionChannel].toServise_handle=xMessageBufferCreateStatic(MaxMsgLength_uartExt_toService+sizeof(size_t),
			uartExt_toService_storageArea, &uartExt_toService_dataStruct);
	//SPIExtension
	static uint8_t spiExt_fromService_storageArea[MSG_BUFFER_LENGTH];
	static StaticMessageBuffer_t spiExt_fromService_dataStruct;
	messageChannels[SPIExtensionChannel].fromServise_handle=xMessageBufferCreateStatic(MSG_BUFFER_LENGTH, spiExt_fromService_storageArea,
			&spiExt_fromService_dataStruct);

	static uint8_t spiExt_toService_storageArea[MSG_BUFFER_LENGTH];
	static StaticMessageBuffer_t spiExt_toService_dataStruct;
	messageChannels[SPIExtensionChannel].toServise_handle=xMessageBufferCreateStatic(MSG_BUFFER_LENGTH, spiExt_toService_storageArea,
			&spiExt_toService_dataStruct);
	//I2CExtension
	static uint8_t i2cExt_fromService_storageArea[MSG_BUFFER_LENGTH];
	static StaticMessageBuffer_t i2cExt_fromService_dataStruct;
	messageChannels[I2CExtensionChannel].fromServise_handle=xMessageBufferCreateStatic(MSG_BUFFER_LENGTH, i2cExt_fromService_storageArea,
			&i2cExt_fromService_dataStruct);

	static uint8_t i2cExt_toService_storageArea[MSG_BUFFER_LENGTH];
	static StaticMessageBuffer_t i2cExt_toService_dataStruct;
	messageChannels[I2CExtensionChannel].toServise_handle=xMessageBufferCreateStatic(MSG_BUFFER_LENGTH, i2cExt_toService_storageArea,
			&i2cExt_toService_dataStruct);
	//StaskMonitor
	static uint8_t stkMon_fromService_storageArea[MSG_BUFFER_LENGTH];
	static StaticMessageBuffer_t stkMon_fromService_dataStruct;
	messageChannels[StackMonitorChannel].fromServise_handle=xMessageBufferCreateStatic(MSG_BUFFER_LENGTH, stkMon_fromService_storageArea,
			&stkMon_fromService_dataStruct);

	static uint8_t stkMon_toService_storageArea[MSG_BUFFER_LENGTH];
	static StaticMessageBuffer_t stkMon_toService_dataStruct;
	messageChannels[StackMonitorChannel].toServise_handle=xMessageBufferCreateStatic(MSG_BUFFER_LENGTH, stkMon_toService_storageArea,
			&stkMon_toService_dataStruct);

}

void MessageRouter_mainTask(void *pvParameters){
	uint8_t msgBuffer[MaxMsgLength], msgLength;
	uint32_t msgControl;
	uint32_t source, destination;
	while(1){
		for(source=0; source<NumberOfChannels; source++){
			msgLength=xMessageBufferReceive(messageChannels[source].fromServise_handle, msgBuffer, MaxMsgLength, 0);
			if(0!=msgLength){
				msgControl=*(uint32_t *)msgBuffer;
				destination=(msgControl & MsgDestinationMask)>>MsgDestinationBase;
				if((msgControl & MsgTypeMask)==MsgType_NoSupport){
					//report, that message was not supported
					//TODO: do something
				} else if(BroadCastChannel==destination){
					//some broad cast message
					//TODO: do something
				} else {
					//ordinary message
					*(uint32_t *)msgBuffer |= source<<MsgSourceBase; //fill source
					//TODO: not portMAX_DELAY, let message be lost after some delay; in Message.c too
					xMessageBufferSend(messageChannels[destination].toServise_handle, msgBuffer, msgLength, portMAX_DELAY);
				}
			}
		}
		taskYIELD();
	}
}

TaskHandle_t MessageRouter_Launch(){
	TaskHandle_t mainTask_handle;
	static const char mainTask_name[configMAX_TASK_NAME_LEN]="MessageRouter";
	static StaticTask_t mainTask_TCB;
	static StackType_t mainTask_stackBuffer[128]; //length=mainTask_stackDepth

	mainTask_handle=xTaskCreateStatic(MessageRouter_mainTask,
				mainTask_name,
				128, //mainTask_stackDepth
				(void *)0,
				1,
				mainTask_stackBuffer,
				&mainTask_TCB);

	return mainTask_handle;
}
