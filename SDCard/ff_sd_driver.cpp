//
// Created by independent-variable on 5/4/2024.
//
#include "ff_sd_driver.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include "periph_allocation.h"
#include "sd_driver.h"
#include "sd_errors.h"

#if SD_IRQ_PRIORITY < configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY \
	|| COARSE_TIMER_IRQ_PRIORITY < configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
#error FatFs driver of SD uses FreeRTOS API
#endif

static StaticEventGroup_t events_ctrl;
static EventGroupHandle_t events;
/** For debug. */
static volatile sd::Error last_read_write_error = sd::Error::NONE;

#define RW_DONE					(1U<<0)
#define RW_FAILED_PROTECTION	(1U<<1)
#define RW_FAILED_ADDRESS		(1U<<2)
#define RW_FAILED				(1U<<3)
#define RW_EVENTS				(RW_DONE | RW_FAILED_PROTECTION | RW_FAILED_ADDRESS | RW_FAILED)

void sd::init_disk_driver() {
	events = xEventGroupCreateStatic(&events_ctrl);
}

DSTATUS sd::disk_initialize () {
	// already initialized
	return 0;
}

DSTATUS sd::disk_status () {
	if (!is_card_present()) {
		return STA_NODISK;
	} else if (is_write_protected()) {
		return STA_PROTECT;
	} else {
		return 0;
	}
}

static void read_write_done(uint32_t block_count, sd::Error error) {
	using namespace sd;
	EventBits_t event;
	if (error == Error::NONE) {
		event = RW_DONE;
	} else {
		last_read_write_error = error;
		if (error == Error::ADDRESS_ERROR || error == Error::ARG_OUT_OF_RANGE) {
			event = RW_FAILED_ADDRESS;
		} else if (error == Error::WP_VIOLATION) {
			event = RW_FAILED_PROTECTION;
		} else {
			event = RW_FAILED;
		}
	}
	BaseType_t task_woken = pdFALSE;
	xEventGroupSetBitsFromISR(events, event, &task_woken);
	portYIELD_FROM_ISR(task_woken);
}

static inline DRESULT events_to_result(EventBits_t bits) {
	if (bits & RW_FAILED_PROTECTION) {
		return RES_WRPRT;
	} else if (bits & RW_FAILED_ADDRESS) {
		return RES_PARERR;
	} else if (bits & RW_FAILED) {
		return RES_ERROR;
	} else {
		return RES_OK;
	}
}

DRESULT sd::disk_read (BYTE* buff, DWORD sector, UINT count) {
	read(sector, count, buff, read_write_done);
	EventBits_t bits;
	const BaseType_t clear_on_exit = pdTRUE, wait_any_bit = pdFALSE;
	while ((bits = xEventGroupWaitBits(events, RW_EVENTS, clear_on_exit, wait_any_bit, portMAX_DELAY)) == 0);
	return events_to_result(bits);
}

DRESULT sd::disk_write (const BYTE* buff, DWORD sector, UINT count) {
	write(sector, count, buff, read_write_done);
	EventBits_t bits;
	const BaseType_t clear_on_exit = pdTRUE, wait_any_bit = pdFALSE;
	while ((bits = xEventGroupWaitBits(events, RW_EVENTS, clear_on_exit, wait_any_bit, portMAX_DELAY)) == 0);
	return events_to_result(bits);
}

DRESULT sd::disk_ioctl (BYTE cmd, void* buff) {
	switch (cmd) {
		case CTRL_SYNC:
			/* Complete pending write process (needed at FF_FS_READONLY == 0) */
			return RES_OK;

		case GET_SECTOR_COUNT:
			/* Get media size (needed at FF_USE_MKFS == 1) */
			*(LBA_t *)buff = get_block_count();
			return RES_OK;

		case GET_SECTOR_SIZE:
			/* Get sector size (needed at FF_MAX_SS != FF_MIN_SS) */
			*(DWORD *)buff = block_len;
			return RES_OK;

		case GET_BLOCK_SIZE:
			/* Get erase block size (needed at FF_USE_MKFS == 1) */
			*(DWORD * )buff = 1;
			return RES_OK;

		case CTRL_TRIM:
			/* Inform device that the data on the block of sectors is no longer used (needed at FF_USE_TRIM == 1) */
			return RES_OK;

		default:
			return RES_PARERR;
	}
}