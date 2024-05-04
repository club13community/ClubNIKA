//
// Created by independent-variable on 4/23/2024.
//
#include "cmd_execution.h"
#include "cmd_codes.h"
#include "stm32f10x.h"
#include "card_registers.h"

using namespace sd;

static union {
	Callback callback;
	OCR_Consumer ocr_consumer;
	RCA_Consumer rca_consumer;
	CSR_Consumer csr_consumer;
} volatile on_done;
static uint8_t * volatile buffer;
static void (* volatile irq_handler)(Error comm_error);

static volatile CSR_Consumer on_app_cmd;
static volatile uint32_t acmd_arg;

#define CMD_NO_RESP		(SDIO_CMD_NIEN | SDIO_CMD_CPSMEN)
#define CMD_SHORT_RESP	(SDIO_CMD_NIEN | SDIO_CMD_CPSMEN | SDIO_CMD_WAITRESP_0)
#define CMD_LONG_RESP	(SDIO_CMD_NIEN | SDIO_CMD_CPSMEN | SDIO_CMD_WAITRESP_1 | SDIO_CMD_WAITRESP_0)

/** Enable "command sent" interrupt*/
#define MASK_FOR_COMMAND	SDIO_MASK_CMDSENTIE
/** Enable "response received", "wrong CRC" and "no response" interrupts */
#define MASK_FOR_RESPONSE	(SDIO_MASK_CMDRENDIE | SDIO_MASK_CTIMEOUTIE | SDIO_MASK_CCRCFAILIE)
/** All used 'enable interrupt', 'clear interrupt' and 'interrupt' flags */
#define CMD_PATH_FLAGS		(MASK_FOR_COMMAND | MASK_FOR_RESPONSE)

#define MASK(mask)			(SDIO->MASK = SDIO->MASK & ~CMD_PATH_FLAGS | (mask))

/** Handles R1 response: if no errors in CSR - invokes callback with CSR */
static void r1_irq_handler(Error comm_error) {
	if (comm_error == Error::NONE) {
		CSR_t csr = CSR_t(SDIO->RESP1);
		on_done.csr_consumer(csr, csr.any_error());
	} else {
		on_done.csr_consumer(CSR_t(0), comm_error);
	}
}

static inline void prepare_for_r1(CSR_Consumer callback) {
	on_done.csr_consumer = callback;
	irq_handler = r1_irq_handler;
}

/** Handles R2 response(with CID or CSD), by copying data to buffer and invoking no-arg callback */
static void r2_irq_handler(Error comm_error) {
	if (comm_error == Error::NONE) {
		if (buffer != nullptr) {
			((uint32_t *)buffer)[0] = SDIO->RESP4;
			((uint32_t *)buffer)[1] = SDIO->RESP3;
			((uint32_t *)buffer)[2] = SDIO->RESP2;
			((uint32_t *)buffer)[3] = SDIO->RESP1;
		}
		on_done.callback(Error::NONE);
	} else {
		on_done.callback(comm_error);
	}
}

static inline void prepare_for_r2(Callback callback, uint8_t * buff) {
	on_done.callback = callback;
	irq_handler = r2_irq_handler;
	buffer = buff;
}

static void cmd55_irq_handler(Error comm_error) {
	if (comm_error == Error::NONE) {
		CSR_t csr = CSR_t(SDIO->RESP1);
		on_app_cmd(csr, csr.any_error());
	} else {
		on_app_cmd(CSR_t(0), comm_error);
	}
}

static inline void exe_cmd55(uint16_t rca, CSR_Consumer do_next, uint32_t arg) {
	on_app_cmd = do_next;
	acmd_arg = arg;
	irq_handler = cmd55_irq_handler;

	MASK(MASK_FOR_RESPONSE);
	SDIO->ARG = (uint32_t)rca << 16;
	SDIO->CMD = CMD_SHORT_RESP | CMD55_APP_CMD;
}

static void cmd0_irq_handler(Error comm_error) {
	on_done.callback(comm_error);
}

/** GO_IDLE_STATE */
void sd::exe_cmd0(Callback callback) {
	on_done.callback = callback;
	irq_handler = cmd0_irq_handler;

	MASK(MASK_FOR_COMMAND);
	SDIO->ARG = 0;
	SDIO->CMD = CMD_NO_RESP | CMD0_GO_IDLE_STATE;
}

static void cmd8_irq_handler(Error comm_error) {
	if (comm_error == Error::NONE) {
		if ((SDIO->RESP1 & 0x1FF) == ((1 << 8) | 0xAA)) {
			on_done.callback(Error::NONE);
		} else {
			on_done.callback(Error::NOT_SUPPORTED_VDD);
		}
	} else {
		on_done.callback(comm_error);
	}
}

/** SEND_IF_COND. Sets "supplied voltage" = 2.7-3.6V, does not request anything related to PCIe */
void sd::exe_cmd8(Callback callback) {
	on_done.callback = callback;
	irq_handler = cmd8_irq_handler;

	MASK(MASK_FOR_RESPONSE);
	SDIO->ARG = (1 << 8) | 0xAA; // 0xAA - check pattern
	SDIO->CMD = CMD_SHORT_RESP | CMD8_SEND_IF_COND;
}

static void acmd41_irq_handler(Error comm_error) {
	// R3 does not have valid CRC
	if (comm_error == Error::RESP_CRC_ERROR) {
		OCR_t ocr = OCR_t(SDIO->RESP1);
		on_done.ocr_consumer(ocr, Error::NONE);
	} else {
		on_done.ocr_consumer(OCR_t(0), comm_error == Error::NONE ? Error::RESP_CRC_ERROR : comm_error);
	}
}

static void do_exe_acmd41(CSR_t csr, Error error) {
	if (error == Error::NONE) {
		irq_handler = acmd41_irq_handler;

		MASK(MASK_FOR_RESPONSE);
		SDIO->ARG = acmd_arg;
		SDIO->CMD = CMD_SHORT_RESP | ACMD41_SD_SEND_OP_COND;
	} else {
		on_done.ocr_consumer(OCR_t(0), error);
	}
}

/** SEND_OP_COND */
void sd::exe_acmd41(CapacitySupport cs, MinVdd min_vdd, MaxVdd max_vdd, OCR_Consumer callback) {
	on_done.ocr_consumer = callback;
	exe_cmd55(0, do_exe_acmd41, (uint32_t)min_vdd & (uint32_t)max_vdd | (uint32_t)cs);
}

/** ALL_SEND_CID.
 * @param cid_ptr points to 16 bytes buffer, CID(with own CRC) will be stored there, may be nullptr */
void sd::exe_cmd2(uint8_t * cid_ptr, Callback callback) {
	prepare_for_r2(callback, cid_ptr);

	MASK(MASK_FOR_RESPONSE);
	SDIO->ARG = 0;
	SDIO->CMD = CMD_LONG_RESP | CMD2_ALL_SEND_CID;
}

static void cmd3_irq_handler(Error comm_error) {
	if (comm_error == Error::NONE) {
		uint32_t resp = SDIO->RESP1;
		uint32_t csr_bits = (resp & (1U << 15 | 1U << 14)) << (23 - 15) // [15:14] are [23:22] of CSR
				| (resp & 1U << 13) << (19 - 13) // [13] is [19] of CSR
				| resp & ((1U << 13) - 1); // [12:0] are [12:0] of CSR
		CSR_t csr = CSR_t(csr_bits);
		uint16_t rca = resp >> 16;
		on_done.rca_consumer(rca, csr, csr.any_error());
	} else {
		on_done.rca_consumer(0, CSR_t(0), comm_error);
	}
}

/** SEND_RELATIVE_ADDR */
void sd::exe_cmd3(RCA_Consumer callback) {
	on_done.rca_consumer = callback;
	irq_handler = cmd3_irq_handler;

	MASK(MASK_FOR_RESPONSE);
	SDIO->ARG = 0;
	SDIO->CMD = CMD_SHORT_RESP | CMD3_SEND_RELATIVE_ADDR;
}

/** SEND_STATUS. Requests card's status. Querying status of task is not supported. */
void sd::exe_cmd13(uint16_t rca, CSR_Consumer callback) {
	prepare_for_r1(callback);

	MASK(MASK_FOR_RESPONSE);
	SDIO->ARG = (uint32_t)rca << 16;
	SDIO->CMD = CMD_SHORT_RESP | CMD13_SEND_STATUS;
}

/** SEND_CSD
* @param csd_ptr points to 16 bytes buffer, CSD(with own CRC) will be stored there, may be nullptr */
void sd::exe_cmd9(uint16_t rca, uint8_t * csd_ptr, Callback callback) {
	prepare_for_r2(callback, csd_ptr);

	MASK(MASK_FOR_RESPONSE);
	SDIO->ARG = (uint32_t)rca << 16;
	SDIO->CMD = CMD_LONG_RESP | CMD9_SEND_CSD;
}

/** SELECT/DESELECT_CARD */
void sd::exe_cmd7(uint16_t rca, CSR_Consumer callback) {
	prepare_for_r1(callback);

	MASK(MASK_FOR_RESPONSE);
	SDIO->ARG = (uint32_t)rca << 16;
	SDIO->CMD = CMD_SHORT_RESP | CMD7_SELECT_DESELECT_CARD;
}

static void do_exe_acmd6(CSR_t csr, Error error) {
	if (error == Error::NONE) {
		irq_handler = r1_irq_handler;

		MASK(MASK_FOR_RESPONSE);
		SDIO->ARG = acmd_arg;
		SDIO->CMD = CMD_SHORT_RESP | ACMD6_SET_BUS_WIDTH;
	} else {
		on_done.csr_consumer(csr, error);
	}
}

/** SET_BUS_WIDTH */
void sd::exe_acmd6(BusWidth width, CSR_Consumer callback) {
	on_done.csr_consumer = callback;
	exe_cmd55(0, do_exe_acmd6, (uint32_t)width);
}

static void do_exe_acmd42(CSR_t csr, Error error) {
	if (error == Error::NONE) {
		irq_handler = r1_irq_handler;

		MASK(MASK_FOR_RESPONSE);
		SDIO->ARG = acmd_arg;
		SDIO->CMD = CMD_SHORT_RESP | ACMD42_SET_CLR_CARD_DETECT;
	} else {
		on_done.csr_consumer(csr, error);
	}
}

/** SET_CLD_CARD_DETECT */
void sd::exe_acmd42(DAT3_PullUp state, CSR_Consumer callback) {
	on_done.csr_consumer = callback;
	exe_cmd55(0, do_exe_acmd42, (uint32_t)state);
}

/** WRITE_BLOCK
 * @param addr byte address for SC card, block address for HC/XC card */
void sd::exe_cmd24(uint32_t addr, CSR_Consumer callback) {
	prepare_for_r1(callback);

	MASK(MASK_FOR_RESPONSE);
	SDIO->ARG = addr;
	SDIO->CMD = CMD_SHORT_RESP | CMD24_WRITE_BLOCK;
}

/** READ_SINGLE_BLOCK
 * @param addr byte address for SC card, block address for HC/XC card */
void sd::exe_cmd17(uint32_t addr, CSR_Consumer callback) {
	prepare_for_r1(callback);

	MASK(MASK_FOR_RESPONSE);
	SDIO->ARG = addr;
	SDIO->CMD = CMD_SHORT_RESP | CMD17_READ_SINGLE_BLOCK;
}

/** WRITE_MULTIPLE_BLOCK */
void sd::exe_cmd25(uint32_t addr, CSR_Consumer callback) {
	prepare_for_r1(callback);

	MASK(MASK_FOR_RESPONSE);
	SDIO->ARG = addr;
	SDIO->CMD = CMD_SHORT_RESP | CMD25_WRITE_MULTIPLE_BLOCK;
}

/** READ_MULTIPLE_BLOCK */
void sd::exe_cmd18(uint32_t addr, CSR_Consumer callback) {
	prepare_for_r1(callback);

	MASK(MASK_FOR_RESPONSE);
	SDIO->ARG = addr;
	SDIO->CMD = CMD_SHORT_RESP | CMD18_READ_MULTIPLE_BLOCK;
}

/** STOP_TRANSMISSION */
void sd::pend_exe_cmd12(CSR_Consumer callback) {
	prepare_for_r1(callback);

	MASK(MASK_FOR_RESPONSE);
	SDIO->ARG = 0;
	SDIO->CMD = CMD_SHORT_RESP | SDIO_CMD_WAITPEND | CMD12_STOP_TRANSMISSION;
}

void sd::handle_cmd_irq(uint32_t sta) {
	sta &= CMD_PATH_FLAGS;
	SDIO->ICR = CMD_PATH_FLAGS;
	if (sta & SDIO_STA_CTIMEOUT) {
		irq_handler(Error::NO_RESPONSE);
	} else if (sta & SDIO_STA_CCRCFAIL) {
		irq_handler(Error::RESP_CRC_ERROR);
	} else if (sta) {
		irq_handler(Error::NONE);
	}
}