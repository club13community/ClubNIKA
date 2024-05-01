//
// Created by independent-variable on 4/27/2024.
//
#include "sd.h"
#include "sd_info_private.h"
#include "data_exchange.h"
#include "periph.h"

using namespace sd;

static volatile Callback on_done;

/** For single or multiple block write */
#define MASK_BLOCK_WRITE	(SDIO_MASK_DATAENDIE | SDIO_MASK_DCRCFAILIE | SDIO_MASK_TXUNDERRIE | SDIO_MASK_DTIMEOUTIE)
/** For single or multiple block read */
#define MASK_BLOCK_READ		(SDIO_MASK_DATAENDIE | SDIO_MASK_DCRCFAILIE | SDIO_MASK_RXOVERRIE | SDIO_MASK_DTIMEOUTIE)
/** All used 'enable interrupt', 'clear interrupt' and 'interrupt' flags */
#define DATA_PATH_FLAGS	(MASK_BLOCK_READ | MASK_BLOCK_WRITE)

#define DCTRL_BLOCK 	(SDIO_DCTRL_DBLOCKSIZE_3 | SDIO_DCTRL_DBLOCKSIZE_0)
#define DCTRL_WRITE		(SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTEN)
#define DCTRL_READ		(SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTEN | SDIO_DCTRL_DTDIR)
/** Disable data-path */
#define DCTRL_TO_IDLE	0U

#define MASK(mask)	(SDIO->MASK = SDIO->MASK & ~DATA_PATH_FLAGS | (mask))

void sd::send(uint8_t * buff, Callback callback) {
	on_done = callback;

	MASK(MASK_BLOCK_WRITE);
	SDIO->DLEN = block_len;
	tx_via_dma((uint32_t *) buff, block_len >> 2);
	SDIO->DCTRL = DCTRL_BLOCK | DCTRL_WRITE;
}

void sd::receive(uint8_t * buff, Callback callback) {
	on_done = callback;

	MASK(MASK_BLOCK_READ);
	SDIO->DLEN = block_len;
	rx_via_dma((uint32_t *)buff, block_len >> 2);
	SDIO->DCTRL = DCTRL_BLOCK | DCTRL_READ;
}

void sd::cancel_receive() {
	SDIO->DCTRL = DCTRL_TO_IDLE;
	stop_dma();
	SDIO->ICR = DATA_PATH_FLAGS;
}

void sd::handle_data_irq(uint32_t sta) {
	sta &= DATA_PATH_FLAGS;
	SDIO->ICR = DATA_PATH_FLAGS;
	if (sta & SDIO_STA_DTIMEOUT) {
		// data was not received
		stop_dma();
		on_done(Error::DATA_TIMEOUT);
		return;
	}
	if (sta & (SDIO_STA_RXOVERR | SDIO_STA_TXUNDERR)) {
		// FIFO over/under ran
		stop_dma();
		on_done(Error::FIFO_ERROR);
		return;
	}
	if (sta & SDIO_STA_DCRCFAIL) {
		// CRC failed
		stop_dma();
		on_done(Error::DATA_CRC_ERROR);
		return;
	}
	if (sta & SDIO_STA_DATAEND) {
		// Block sent received
		stop_dma();
		on_done(Error::NONE);
		return;
	}
}