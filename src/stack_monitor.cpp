//
// Created by independent-variable on 5/5/2024.
//
#include "periph_allocation.h"
#include "FreeRTOS.h"
#include "task.h"
#include "logging.h"

#define MONITORING_PERIOD_s		30U
#define MAX_TASK_NUM			30U
#define STACK_SIZE				256U

static StackType_t stack[STACK_SIZE];
static StaticTask_t ctrl;
static TaskHandle_t handle;

static TaskStatus_t task_statuses[MAX_TASK_NUM];

static void monitor_stack(void * args) {
	using rec::s, rec::log;
	while (true) {
		uint32_t tot_run_time;
		UBaseType_t num = uxTaskGetSystemState(task_statuses, MAX_TASK_NUM, &tot_run_time);
		if (num == 0) {
			// too many tasks
			continue;
		}
		for (UBaseType_t i = 0; i < num; i++) {
			TaskStatus_t status = task_statuses[i];
			size_t total_stack = status.pxEndOfStack - status.pxStackBase + 1;
			size_t used_stack = total_stack - status.usStackHighWaterMark;
			uint8_t stack_usage = (uint32_t)used_stack * 100U / total_stack;
			log("'{0}' uses {1}% of stack", {status.pcTaskName, s(stack_usage)});
		}
		vTaskDelay(pdMS_TO_TICKS(MONITORING_PERIOD_s * 1000U));
	}
}

void start_stack_monitor() {
	handle = xTaskCreateStatic(monitor_stack, "stack monitor",
							   STACK_SIZE, nullptr, TASK_NORMAL_PRIORITY, stack, &ctrl);
}

