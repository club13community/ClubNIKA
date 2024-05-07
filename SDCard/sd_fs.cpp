//
// Created by independent-variable on 5/4/2024.
//
#include "sd_fs.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "periph_allocation.h"
#include "sd_driver.h"
#include "sd_driver_callbacks.h"
#include "ff_sd_driver.h"
#include "drives.h"

static StaticEventGroup_t events_ctrl;
static EventGroupHandle_t events;
#define CARD_INSERTED	(1U << 0)
#define CARD_REMOVED	(1U << 1)

#define MOUNT_TASK_STACK_DEPTH		256U
static StackType_t mount_task_stack[MOUNT_TASK_STACK_DEPTH];
static StaticTask_t mount_task_ctrl;
static TaskHandle_t mount_task;

void sd::on_card_inserted() {
	BaseType_t task_woken = pdFALSE;
	xEventGroupSetBitsFromISR(events, CARD_INSERTED, &task_woken);
	portYIELD_FROM_ISR(task_woken);
}

void sd::on_card_removed() {
	BaseType_t task_woken = pdFALSE;
	xEventGroupSetBitsFromISR(events, CARD_REMOVED, &task_woken);
	portYIELD_FROM_ISR(task_woken);
}

static void serve_mounting(void * arg) {
	using namespace sd;
	start_driver();
	const BaseType_t clear_on_exit = pdTRUE, wait_for_any = pdFALSE;
	EventBits_t bits;
	while (true) {
		while ((bits = xEventGroupWaitBits(events, CARD_INSERTED | CARD_REMOVED,
										   clear_on_exit, wait_for_any, portMAX_DELAY)) == 0);
		if (bits == CARD_INSERTED) {
			f_mount(&sd_fs, "/sd", 1);
		} else if (bits == CARD_REMOVED) {
			f_unmount("/sd");
		} else {
			// practically impossible case, card quickly inserted and removed - remount if present
			f_unmount("/sd");
			if (is_card_present()) {
				f_mount(&sd_fs, "/sd", 1);
			}
		}
	}
}

/** Prepares everything for file system operations on SD card. Operations will start after FreeRTOS planner starts */
void sd::start() {
	init_driver();
	init_disk_driver();
	events = xEventGroupCreateStatic(&events_ctrl);
	mount_task = xTaskCreateStatic(serve_mounting, "SD service", MOUNT_TASK_STACK_DEPTH, nullptr, SD_SERVICE_PRIORITY,
								   mount_task_stack, &mount_task_ctrl);
	mount_task = mount_task;
}