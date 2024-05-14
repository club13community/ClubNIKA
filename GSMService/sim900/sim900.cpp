//
// Created by independent-variable on 5/11/2024.
//
#include "sim900.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "./execution.h"
#include "./power_ctrl.h"
#include "./uart_ctrl.h"
#include "./call_ctrl.h"
#include "./sms_ctrl.h"
#include "./status_ctrl.h"

using namespace sim900;

static SemaphoreHandle_t ctrl_mutex;

void sim900::init_periph() {
	init_power_ctrl();
	init_uart_ctrl();
}

void sim900::start() {
	static StaticSemaphore_t mutex_ctrl;
	ctrl_mutex = xSemaphoreCreateBinaryStatic(&mutex_ctrl);
	start_execution();
}