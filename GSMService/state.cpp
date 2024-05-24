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

	void (* volatile on_call_ended)() = nullptr;

	volatile sim900::CardStatus card_status = sim900::CardStatus::ERROR;
	volatile sim900::Registration registration = sim900::Registration::ONGOING;
	volatile uint8_t signal_strength = 0;

	volatile sim900::CallState actual_call_state = sim900::CallState::ENDED;
	volatile sim900::CallState handled_call_state = sim900::CallState::ENDED;
	volatile sim900::CallDirection call_direction = sim900::CallDirection::INCOMING;
	char phone_number[MAX_PHONE_LENGTH + 1];

	volatile QueueHandle_t result_queue;

	volatile SemaphoreHandle_t ctrl_mutex;
	volatile SemaphoreHandle_t call_mutex;

	void init_state() {
		static StaticSemaphore_t ctrl_mutex_buff;
		ctrl_mutex = xSemaphoreCreateBinaryStatic(&ctrl_mutex_buff);
		// mutex stays 'taken' till GSM module turned on

		static StaticSemaphore_t call_mutex_buff;
		call_mutex = xSemaphoreCreateBinaryStatic(&call_mutex_buff);
		xSemaphoreGive(call_mutex);

		static uint8_t result_queue_data[sizeof (FutureResult)];
		static StaticQueue_t result_queue_ctrl;
		result_queue = xQueueCreateStatic(1, sizeof (FutureResult), result_queue_data, &result_queue_ctrl);
	}


	void reboot_state() {
		card_status = sim900::CardStatus::ERROR;
		registration = sim900::Registration::ONGOING;
		signal_strength = 0;

		actual_call_state = sim900::CallState::ENDED;
		handled_call_state = sim900::CallState::ENDED;
		call_direction = sim900::CallDirection::INCOMING;

		// don't reset result queue - this may block thread which waits for operation to end.
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