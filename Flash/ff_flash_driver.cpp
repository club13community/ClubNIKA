//
// Created by independent-variable on 4/15/2024.
//
#include "ff_flash_driver.h"
#include "flash.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include "periph_allocation.h"

#if FLASH_IRQ_PRIORITY < configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
#error FatFs driver of Flash uses FreeRTOS API
#endif

static EventGroupHandle_t events;
static StaticEventGroup_t events_ctrl;
#define RW_DONE			(1U << 0)
#define RW_FAILED		(1U << 1)
#define ALL_RW_EVENTS	(RW_DONE | RW_FAILED)

void flash::init_disk_driver() {
	events = xEventGroupCreateStatic(&events_ctrl);
}

DSTATUS flash::disk_initialize () {
	return 0; // already initialized
}

DSTATUS flash::disk_status () {
	return 0; // all ok
}

static void set_all_done(bool success) {
	BaseType_t ptw = pdFALSE;
	xEventGroupSetBitsFromISR(events, success ? RW_DONE : RW_FAILED, &ptw);
	portYIELD_FROM_ISR(ptw);
}

static volatile flash::PageAddress page;
static volatile uint8_t * data;
static volatile uint16_t page_count;

static void read_page(bool prev_success) {
	if (prev_success) {
		flash::PageAddress p = page;
		uint8_t * d = (uint8_t *) data;
		page += 1;
		data += 512;
		bool all_done = --page_count == 0;
		flash::read_memory({.page = p, .byte = 0}, 512, d, all_done ? set_all_done : read_page);
	} else {
		set_all_done(false);
	}
}

DRESULT flash::disk_read (BYTE* buff, DWORD sector, UINT count) {
	page = sector;
	data = buff;
	page_count = count;
	read_page(true);
	const BaseType_t clear_on_exit = pdTRUE, wait_any = pdFALSE;
	EventBits_t bits;
	do {
		bits = xEventGroupWaitBits(events, ALL_RW_EVENTS, clear_on_exit, wait_any, portMAX_DELAY);
	} while (bits == 0);
	return bits & RW_DONE ? RES_OK : RES_ERROR;
}
static void data_to_buffer(bool prev_success);

static void buffer_to_memory(bool prev_success) {
	if (prev_success) {
		uint16_t p = page;
		page += 1;
		data += 512;
		bool all_done = --page_count == 0;
		flash::erase_and_program(flash::Buffer::B1, p, all_done ? set_all_done : data_to_buffer);
	} else {
		set_all_done(false);
	}
}

static void data_to_buffer(bool prev_success) {
	if (prev_success) {
		flash::write_buffer(flash::Buffer::B1, 0, 512, (uint8_t *) data, buffer_to_memory);
	} else {
		set_all_done(false);
	}
}

DRESULT flash::disk_write (const BYTE* buff, DWORD sector, UINT count) {
	page = sector;
	data = (uint8_t *)buff;
	page_count = count;
	data_to_buffer(true);
	const BaseType_t clear_on_exit = pdTRUE, wait_any = pdFALSE;
	EventBits_t bits;
	do {
		bits = xEventGroupWaitBits(events, ALL_RW_EVENTS, clear_on_exit, wait_any, portMAX_DELAY);
	} while (bits == 0);
	return bits & RW_DONE ? RES_OK : RES_ERROR;
}

DRESULT flash::disk_ioctl (BYTE cmd, void* buff) {
	switch (cmd) {
		case CTRL_SYNC:
			/* Complete pending write process (needed at FF_FS_READONLY == 0) */
			return RES_OK;

		case GET_SECTOR_COUNT:
			/* Get media size (needed at FF_USE_MKFS == 1) */
			*(LBA_t *)buff = 8192;
			return RES_OK;

		case GET_SECTOR_SIZE:
			/* Get sector size (needed at FF_MAX_SS != FF_MIN_SS) */
			*(DWORD *)buff = 512;
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