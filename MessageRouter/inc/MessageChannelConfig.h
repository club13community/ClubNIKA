/*
 * MessageChannelConfig.h
 *
 *  Created on: 4 трав. 2020 р.
 *      Author: MaxCm
 */

#ifndef _MESSAGECHANNELCONFIG_H_
#define _MESSAGECHANNELCONFIG_H_
#include "Message.h"

#define MaxMsgLength_supSys_fromService		MaxMsgLength
#define MaxMsgLength_supSys_toService		5

#define MaxMsgLength_voltMet_fromService	MaxMsgLength
#define MaxMsgLength_voltMet_toService		5

#define MaxMsgLength_uartExt_fromService	MaxMsgLength
#define MaxMsgLength_uartExt_toService		MaxMsgLength



#define SupplySystemChannel			0
#define ClockControlChannel			1
#define WiredSensorMonitorChannel	2
#define VoltageMeterChannel			3
#define GSMServiceChannel			4
#define WirelessInterfaceChannel	5
#define UserInterfaceChannel 		6
#define SoundServiceChannel			7
#define UARTExtensionChannel		8
#define SPIExtensionChannel			9
#define I2CExtensionChannel			10
#ifndef DEBUG
#define NumberOfChannels			11
#else
#define StackMonitorChannel			11
#define NumberOfChannels			12
#endif
#define BroadCastChannel			0x1F

#endif /* _MESSAGECHANNELCONFIG_H_ */
