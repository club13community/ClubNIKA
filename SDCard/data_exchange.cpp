//
// Created by independent-variable on 4/27/2024.
//
#include "sd.h"
#include "sd_info_private.h"
#include "cmd_execution.h"
#include "data_exchange.h"
#include "periph.h"

using namespace sd;

#define BLOCK_LEN	512U
static uint8_t * volatile buff;
static void (* volatile callback)(Error);

static inline uint32_t to_addr(uint32_t block) {
	return sd::hcs == CapacitySupport::SC ? block << 9 : block;
}

static void xchg_cmd_failed(Error error) {
	callback(error);
}

/** For single or multiple block write */
#define MASK_BLOCK_WRITE	(SDIO_MASK_DBCKENDIE | SDIO_MASK_DCRCFAILIE | SDIO_MASK_TXUNDERRIE | SDIO_MASK_DTIMEOUTIE)
/** For single or multiple block read */
#define MASK_BLOCK_READ		(SDIO_MASK_DBCKENDIE | SDIO_MASK_DCRCFAILIE | SDIO_MASK_RXOVERRIE | SDIO_MASK_DTIMEOUTIE)
/** All used 'interrupt enable' flags */
#define MASK_DATA_PATH	(MASK_BLOCK_READ | MASK_BLOCK_WRITE)
#define ICR_DATA_PATH	(SDIO_ICR_DBCKENDC | SDIO_ICR_DCRCFAILC | SDIO_ICR_RXOVERRC | SDIO_ICR_TXUNDERRC \
						| SDIO_ICR_DTIMEOUTC | SDIO_ICR_DATAENDC)

#define DCTRL_BLOCK 	(SDIO_DCTRL_DBLOCKSIZE_3 | SDIO_DCTRL_DBLOCKSIZE_0)
#define DCTRL_WRITE		(SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTEN)
#define DCTRL_READ		(SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTEN | SDIO_DCTRL_DTDIR)
/** Disable data-path */
#define DCTRL_TO_IDLE	0U

#define MASK(mask)	(SDIO->MASK = SDIO->MASK & ~MASK_DATA_PATH | (mask))

static void wr_single_cmd_done(CSR_t csr) {
	MASK(MASK_BLOCK_WRITE);
	SDIO->DLEN = BLOCK_LEN;
	tx_via_dma((uint32_t *)buff, BLOCK_LEN >> 2);
	SDIO->DCTRL = DCTRL_BLOCK | DCTRL_WRITE;
}

void sd::write_block(uint32_t block, uint8_t * buff, void (* callback)(Error)) {
	::buff = buff;
	::callback = callback;
	exe_cmd24(to_addr(block), wr_single_cmd_done, xchg_cmd_failed);
}

static void rd_single_cmd_done(CSR_t csr) {
	// nothing to do
}

static void read_cmd_failed(Error error) {
	SDIO->DCTRL = DCTRL_TO_IDLE;
	stop_dma();
	callback(error);
}

void sd::read_block(uint32_t block, uint8_t * buff, void (* callback)(Error)) {
	::buff = buff;
	::callback = callback;

	MASK(MASK_BLOCK_READ);
	SDIO->DLEN = BLOCK_LEN;
	rx_via_dma((uint32_t *)buff, BLOCK_LEN >> 2);
	SDIO->DCTRL = DCTRL_BLOCK | DCTRL_READ;

	exe_cmd17(to_addr(block), rd_single_cmd_done, xchg_cmd_failed);
}

void sd::handle_data_irq(uint32_t sta) {
	if (sta & SDIO_STA_DTIMEOUT) {
		// data was not received
		SDIO->ICR = ICR_DATA_PATH;
		stop_dma();
		callback(Error::DATA_TIMEOUT);
		return;
	}
	if (sta & (SDIO_STA_RXOVERR | SDIO_STA_TXUNDERR)) {
		// FIFO over/under ran
		SDIO->ICR = ICR_DATA_PATH;
		stop_dma();
		callback(Error::FIFO_ERROR);
		return;
	}
	if (sta & SDIO_STA_DCRCFAIL) {
		// CRC failed
		SDIO->ICR = ICR_DATA_PATH;
		stop_dma();
		callback(Error::DATA_CRC_ERROR);
		return;
	}
	if (sta & SDIO_STA_DBCKEND) {
		// Block sent received
		SDIO->ICR = ICR_DATA_PATH;
		stop_dma();
		callback(NO_ERROR);
		return;
	}
}