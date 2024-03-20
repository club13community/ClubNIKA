#ifndef _WIRELESSINTERFACE_H_
#define _WIRELESSINTERFACE_H_

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

void WirelessInterface_CommFinished_IH();

TaskHandle_t WirelessInterface_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut);

#ifdef __cplusplus
}
#endif
#endif //_WIRELESSINTERFACE_H_
