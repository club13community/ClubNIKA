/*
 * GSMService.c
 *
 *  Created on: 3 квіт. 2020 р.
 *      Author: MaxCm
 */
#include "GSMService.h"
#include "sim900.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "./CtrlsSet.h"
#include "./EndCtrls.h"
#include "./execution.h"

static SemaphoreHandle_t ctrl_mutex;

void gsm::init_periph() {
	sim900::init_periph();
}

void gsm::start() {
	static StaticSemaphore_t semaphoreBuffer;
	ctrl_mutex = xSemaphoreCreateBinaryStatic(&semaphoreBuffer);
	xSemaphoreGive(ctrl_mutex);
	sim900::start();
}

gsm::Controls & gsm::get_ctrl() {
	while (xSemaphoreTake(ctrl_mutex, portMAX_DELAY) == pdFALSE);
	return CtrlsSet::inst;
}


static volatile union {
	gsm::Handler handler;
} end;

void gsm::set_end(Handler callback) {
	end.handler = callback;
}

void gsm::end_handler() {
	EndCtrls ctrls;
	end.handler(ctrls);
	if (ctrls.release_ctrl()) {
		xSemaphoreGive(ctrl_mutex);
	}
}

void on_call_ended() {

}

void on_incoming_call() {

}