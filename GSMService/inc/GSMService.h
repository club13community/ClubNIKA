#ifndef _GSMSERVICE_H_
#define _GSMSERVICE_H_

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

TaskHandle_t GSMService_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut);

void GSMService_UART_IH();

#ifdef __cplusplus
}
#endif


#endif /* _GSMSERVICE_H_ */
