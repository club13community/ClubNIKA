//
// Created by independent-variable on 5/21/2024.
//
#include "./state.h"
#include "sim900.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"

namespace gsm {
	void (* volatile on_incoming_call)(char *) = nullptr;
	void (* volatile on_call_dialed)(Direction) = nullptr;
	void (* volatile on_call_ended)() = nullptr;

	volatile QueueHandle_t result_queue;

	volatile SemaphoreHandle_t ctrl_mutex;

	void init_state() {
		static StaticSemaphore_t ctrl_mutex_buff;
		ctrl_mutex = xSemaphoreCreateBinaryStatic(&ctrl_mutex_buff);
		xSemaphoreGive(ctrl_mutex);

		static uint8_t result_queue_data[sizeof (FutureResult)];
		static StaticQueue_t result_queue_ctrl;
		result_queue = xQueueCreateStatic(1, sizeof (FutureResult), result_queue_data, &result_queue_ctrl);
	}

	void future_result(FutureResult result) {
		xQueueSend(result_queue, &result, 0); // no delay, reboot procedure may want to put something
	}

	FutureResult future_result() {
		FutureResult result;
		while (xQueueReceive(result_queue, &result, portMAX_DELAY) == pdFALSE);
		return result;
	}
}