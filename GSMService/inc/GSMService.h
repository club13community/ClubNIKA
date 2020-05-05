#ifndef _GSMSERVICE_H_
#define _GSMSERVICE_H_

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"

TaskHandle_t GSMService_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut);
void GSMService_UART_Rx_IH();


#endif /* _GSMSERVICE_H_ */
