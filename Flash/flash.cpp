//
// Created by independent-variable on 3/26/2024.
//
#include "flash.h"
#include "./periph.h"
#include "./utils.h"
#include "./internal.h"
#include "timing.h"

#define TIMER		(timing::fine_timer3)
#define POWER_ON_DELAY			21000U
#define BUFFER_MEMORY_DELAY		310U
#define ERASE_AND_PROGRAM_DELAY	40010U
#define STATUS_RECHECK_DELAY	50U

// command -> data is clocked out
#define READ_STATUS	0xD7U

// speed limit is 33MHz
// command -> 3 address bytes(1bit - any, 13bits - page, 10bits - byte) -> then data is clocked out
#define CONTINUES_ARRAY_READ	0x03U

// speed limit for both is 33MHz
// command -> 3 address bytes(14bits - any, 10bits - address) -> 1 any byte(MISTAKE IN DATASHEET) -> data is clocked out
#define BUFFER_1_READ			0xD1U
#define BUFFER_2_READ			0xD3U

// command -> 3 address bytes(14bits - any, 10bits - address) -> data
#define BUFFER_1_WRITE			0x84U
#define BUFFER_2_WRITE			0x87U

// command -> 3 address bytes(1bit - any, 13bits - page, 10bits - any) -> CS high -> wait 300us -> ready to read buffer
#define MEM_PAGE_TO_BUFFER_1	0x53U
#define MEM_PAGE_TO_BUFFER_2	0x55U

// command -> 3 address byte(1bit - any, 13bits - page, 10bits - any) -> CS high -> wait 17ms..40ms -> programmed
#define ERASE_AND_PROGRAM_BUFFER_1	0x83U
#define ERASE_AND_PROGRAM_BUFFER_2	0x86U

// Bits of STATUS register
#define STATUS_RDY	(1U << 7)

#define ANY_PAGE_ADDRESS	((PageAddress)0U)
#define ANY_BYTE_OFFSET		((ByteOffset)0U)
#define ANY_BYTE			((uint8_t)0U)

using namespace flash;
static volatile Callback on_done;
static volatile uint8_t * data;
static volatile uint16_t count;
static volatile uint8_t ctrl[5];
static volatile State state;

static void read_status();

/** Initializes peripherals and state.
 * Interrupts should be enabled(timer is used for power-on delay, SPI is used to check that fash is ready) */
void flash::init() {
	init_cs();
	TIMER.wait_us(POWER_ON_DELAY);
	init_mosi_miso_sck();
	init_spi();
	state = POWER_UP_CHECK;
	read_status();
	while (state != IDLE);
}

void flash::read_memory(MemoryAddress address, uint16_t length, uint8_t * destination, Callback callback) {
	state = READ_COMMAND;
	data = destination;
	count = length;
	on_done = callback;
	ctrl[0] = CONTINUES_ARRAY_READ;
	put_address_bytes(address.page, address.byte, ctrl + 1);
	cs_select();
	enable_rx_dma(ctrl, 4, MemInc::YES);
	enable_tx_dma(ctrl, 4, MemInc::YES);
}

void flash::read_buffer(Buffer buffer, ByteOffset address, uint16_t length, uint8_t * destination, Callback callback) {
	state = READ_COMMAND;
	data = destination;
	count = length;
	on_done = callback;
	ctrl[0] = buffer == Buffer::B1 ? BUFFER_1_READ : BUFFER_2_READ;
	put_address_bytes(ANY_PAGE_ADDRESS, address, ctrl + 1);
	cs_select();
	enable_rx_dma(ctrl, 4, MemInc::YES);
	enable_tx_dma(ctrl, 4, MemInc::YES);
}

void flash::write_buffer(Buffer buffer, ByteOffset address, uint16_t length, uint8_t * source, Callback callback) {
	state = WRITE_COMMAND;
	data = source;
	count = length;
	on_done = callback;
	ctrl[0] = buffer == Buffer::B1 ? BUFFER_1_WRITE : BUFFER_2_WRITE;
	put_address_bytes(ANY_PAGE_ADDRESS, address, ctrl + 1);
	cs_select();
	enable_rx_dma(ctrl, 4, MemInc::YES);
	enable_tx_dma(ctrl, 4, MemInc::YES);
}

void flash::memory_to_buffer(PageAddress page, Buffer buffer, Callback callback) {
	state = BUFFER_MEMORY_COMMAND;
	on_done = callback;
	ctrl[0] = buffer == Buffer::B1 ? MEM_PAGE_TO_BUFFER_1 : MEM_PAGE_TO_BUFFER_2;
	put_address_bytes(page, ANY_BYTE_OFFSET, ctrl + 1);
	cs_select();
	enable_rx_dma(ctrl, 4, MemInc::YES);
	enable_tx_dma(ctrl, 4, MemInc::YES);
}

void flash::erase_and_program(Buffer buffer, PageAddress page, Callback callback) {
	state = ERASE_AND_PROGRAM_COMMAND;
	on_done = callback;
	ctrl[0] = buffer == Buffer::B1 ? ERASE_AND_PROGRAM_BUFFER_1 : ERASE_AND_PROGRAM_BUFFER_2;
	put_address_bytes(page, ANY_BYTE_OFFSET, ctrl + 1);
	cs_select();
	enable_rx_dma(ctrl, 4, MemInc::YES);
	enable_tx_dma(ctrl, 4, MemInc::YES);
}

static void read_status() {
	ctrl[0] = READ_STATUS;
	ctrl[1] = ANY_BYTE;
	cs_select();
	enable_rx_dma(ctrl, 2, MemInc::YES);
	enable_tx_dma(ctrl, 2, MemInc::YES);
}

static void switch_to_checking() {
	State current = state;
	if (current == BUFFER_MEMORY_EXEC) {
		state = BUFFER_MEMORY_CHECK;
		read_status();
	} else if (current == ERASE_AND_PROGRAM_EXEC) {
		state = ERASE_AND_PROGRAM_CHECK;
		read_status();
	}
}

void flash::dma_isr() {
	disable_tx_dma();
	disable_rx_dma();
	clear_rx_dma_tc();

	State current = const_cast<State &>(state);
	if (current == POWER_UP_CHECK) {
		cs_deselect();
		uint8_t status = ctrl[1];
		if (status & STATUS_RDY) {
			cs_delay();
			state = IDLE;
		} else {
			TIMER.invoke_in_us(STATUS_RECHECK_DELAY, read_status);
		}
	} else if (current == READ_COMMAND) {
		state = READ_DATA;
		enable_rx_dma(data, count, MemInc::YES);
		enable_tx_dma(ctrl, count, MemInc::NO); // just to gen. clk.
	} else if (current == WRITE_COMMAND) {
		state = WRITE_DATA;
		enable_rx_dma(ctrl, count, MemInc::NO); // just to gen. IRQ after all is transferred
		enable_tx_dma(data, count, MemInc::YES);
	} else if (current == READ_DATA || current == WRITE_DATA) {
		state = IDLE;
		cs_deselect();
		cs_delay();
		on_done(true);
	} else if (current == BUFFER_MEMORY_COMMAND) {
		state = BUFFER_MEMORY_EXEC;
		cs_deselect();
		TIMER.invoke_in_us(BUFFER_MEMORY_DELAY, switch_to_checking);
	} else if (current == ERASE_AND_PROGRAM_COMMAND) {
		state = ERASE_AND_PROGRAM_EXEC;
		cs_deselect();
		TIMER.invoke_in_us(ERASE_AND_PROGRAM_DELAY, switch_to_checking);
	} else if (current == BUFFER_MEMORY_CHECK || current == ERASE_AND_PROGRAM_CHECK
				|| current == READY_AFTER_ERROR_CHECK) {
		cs_deselect();
		uint8_t status = ctrl[1];
		if (status & STATUS_RDY) {
			state = IDLE;
			cs_delay();
			on_done(current != READY_AFTER_ERROR_CHECK);
		} else {
			TIMER.invoke_in_us(STATUS_RECHECK_DELAY, read_status);
		}
	}
}

void flash::spi_isr() {
	cs_deselect();
	cs_delay();
	disable_spi();
	disable_tx_dma();
	disable_rx_dma();
	wait_spi_disabled();
	empty_spi_rx_fifo(); // also clears 'overrun flag'
	enable_spi();
	state = READY_AFTER_ERROR_CHECK;
	read_status();
}