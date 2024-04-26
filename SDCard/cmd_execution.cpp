//
// Created by independent-variable on 4/23/2024.
//
#include "cmd_execution.h"
#include "cmd_codes.h"
#include "stm32f10x.h"
#include "sd_registers.h"

using namespace sd;

static union {
	DoneCallback callback;
	OCR_Consumer ocr_consumer;
	RCA_Consumer rca_consumer;
	CSR_Consumer csr_consumer;
} volatile success;
static volatile ErrorCallback error_callback;

static uint8_t * buffer;

static void (* volatile exe_acmd)();
static volatile uint32_t acmd_arg;

static void (* volatile irq_handler)(uint32_t sta);

#define CMD_NO_RESP		(SDIO_CMD_NIEN | SDIO_CMD_CPSMEN)
#define CMD_SHORT_RESP	(SDIO_CMD_NIEN | SDIO_CMD_CPSMEN | SDIO_CMD_WAITRESP_0)
#define CMD_LONG_RESP	(SDIO_CMD_NIEN | SDIO_CMD_CPSMEN | SDIO_CMD_WAITRESP_1 | SDIO_CMD_WAITRESP_0)

/** Enable "command sent" interrupt*/
#define MASK_SEND_COMMAND	SDIO_MASK_CMDSENTIE
/** Enable "response received", "wrong CRC" and "no response" interrupts */
#define MASK_WAIT_RESPONSE	(SDIO_MASK_CMDRENDIE | SDIO_MASK_CTIMEOUTIE | SDIO_MASK_CCRCFAILIE)

#define ICR_CMD_RELATED		(SDIO_ICR_CMDSENTC | SDIO_ICR_CMDRENDC | SDIO_ICR_CTIMEOUTC | SDIO_ICR_CCRCFAILC)

/** Handles R1 response: if no errors in CSR - invokes callback with CSR */
static void r1_irq_handler(uint32_t sta) {
	CSR_t csr = CSR_t(SDIO->RESP1);
	if (csr.has_errors()) {
		error_callback(csr.any_error());
	} else {
		success.csr_consumer(csr);
	}
}

/** Handles R2 response(with CID or CSD), by copying data to buffer and invoking no-arg callback */
static void r2_irq_handler(uint32_t sta) {
	if (buffer != nullptr) {
		((uint32_t *)buffer)[0] = SDIO->RESP4;
		((uint32_t *)buffer)[1] = SDIO->RESP3;
		((uint32_t *)buffer)[2] = SDIO->RESP2;
		((uint32_t *)buffer)[3] = SDIO->RESP1;
	}
	success.callback();
}

static void cmd55_irq_handler(uint32_t sta) {
	CSR_t csr = CSR_t(SDIO->RESP1);
	if (csr.has_errors()) {
		error_callback(csr.any_error());
	} else {
		exe_acmd();
	}
}

static void exe_cmd55(uint16_t rca) {
	irq_handler = cmd55_irq_handler;

	SDIO->MASK = MASK_WAIT_RESPONSE;
	SDIO->ARG = (uint32_t)rca << 16;
	SDIO->CMD = CMD_SHORT_RESP | CMD55_APP_CMD;
}

static void cmd0_irq_handler(uint32_t sta) {
	success.callback();
}

/** GO_IDLE_STATE */
void sd::exe_cmd0(DoneCallback on_done) {
	success.callback = on_done;
	irq_handler = cmd0_irq_handler;

	SDIO->MASK = MASK_SEND_COMMAND;
	SDIO->ARG = 0;
	SDIO->CMD = CMD_NO_RESP | CMD0_GO_IDLE_STATE;
}

static void cmd8_irq_handler(uint32_t sta) {
	if ((SDIO->RESP1 & 0x1FF) != ((1 << 8) | 0xAA)) {
		error_callback(Error::NOT_SUPPORTED_VDD);
	} else {
		success.callback();
	}
}

/** SEND_IF_COND. Sets "supplied voltage" = 2.7-3.6V, does not request anything related to PCIe */
void sd::exe_cmd8(DoneCallback on_done, ErrorCallback on_error) {
	success.callback = on_done;
	error_callback = on_error;
	irq_handler = cmd8_irq_handler;

	SDIO->MASK = MASK_WAIT_RESPONSE;
	SDIO->ARG = (1 << 8) | 0xAA; // 0xAA - check pattern
	SDIO->CMD = CMD_SHORT_RESP | CMD8_SEND_IF_COND;
}

static void acmd41_irq_handler(uint32_t sta) {
	OCR_t ocr = OCR_t(SDIO->RESP1);
	success.ocr_consumer(ocr);
}

static void do_exe_acmd41() {
	irq_handler = acmd41_irq_handler;

	SDIO->MASK = MASK_WAIT_RESPONSE;
	SDIO->ARG = acmd_arg;
	SDIO->CMD = CMD_SHORT_RESP | ACMD41_SD_SEND_OP_COND;
}

/** SEND_OP_COND */
void sd::exe_acmd41(CapacitySupport cs, MinVdd min_vdd, MaxVdd max_vdd, OCR_Consumer on_done, ErrorCallback on_error) {
	success.ocr_consumer = on_done;
	error_callback = on_error;
	exe_acmd = do_exe_acmd41;
	acmd_arg = (uint32_t)min_vdd & (uint32_t)max_vdd | (uint32_t)cs;

	exe_cmd55(0);
}

/** ALL_SEND_CID.
 * @param cid_ptr points to 16 bytes buffer, CID(with own CRC) will be stored there, may be nullptr */
void sd::exe_cmd2(uint8_t * cid_ptr, DoneCallback on_done, ErrorCallback on_error) {
	success.callback = on_done;
	error_callback = on_error;
	buffer = cid_ptr;
	irq_handler = r2_irq_handler;

	SDIO->MASK = MASK_WAIT_RESPONSE;
	SDIO->ARG = 0;
	SDIO->CMD = CMD_LONG_RESP | CMD2_ALL_SEND_CID;
}

static void cmd3_irq_handler(uint32_t sta) {
	uint32_t resp = SDIO->RESP1;
	if (resp & 1U << 15) {
		error_callback(Error::CMD_CRC_ERROR);
	} else if (resp & 1U << 14) {
		error_callback(Error::ILLEGAL_COMMAND);
	} else if (resp & 1U << 13) {
		error_callback(Error::GENERAL_ERROR);
	} else if (resp & 1U << 3) {
		error_callback(Error::AKE_SEQ_ERROR);
	} else {
		uint16_t rca = resp >> 16;
		success.rca_consumer(rca);
	}
}

/** SEND_RELATIVE_ADDR */
void sd::exe_cmd3(RCA_Consumer on_done, ErrorCallback on_error) {
	success.rca_consumer = on_done;
	error_callback = on_error;
	irq_handler = cmd3_irq_handler;

	SDIO->MASK = MASK_WAIT_RESPONSE;
	SDIO->ARG = 0;
	SDIO->CMD = CMD_SHORT_RESP | CMD3_SEND_RELATIVE_ADDR;
}

static void cmd13_irq_handler(uint32_t sta) {
	CSR_t csr = CSR_t(SDIO->RESP1);
	success.csr_consumer(csr);
}

/** SEND_STATUS. Requests card's status. Querying status of task is not supported.
 * @param on_error is NOT invoked if CSR has error flags;
 * it is invoked only in case problems with communication(failed CRC, no response, ets) */
void sd::exe_cmd13(uint16_t rca, CSR_Consumer on_done, ErrorCallback on_error) {
	success.csr_consumer = on_done;
	error_callback = on_error;
	irq_handler = cmd13_irq_handler;

	SDIO->MASK = MASK_WAIT_RESPONSE;
	SDIO->ARG = (uint32_t)rca << 16;
	SDIO->CMD = CMD_SHORT_RESP | CMD13_SEND_STATUS;
}

/** SEND_CSD
* @param csd_ptr points to 16 bytes buffer, CSD(with own CRC) will be stored there, may be nullptr */
void sd::exe_cmd9(uint16_t rca, uint8_t * csd_ptr, DoneCallback on_done, ErrorCallback on_error) {
	success.callback = on_done;
	error_callback = on_error;
	buffer = csd_ptr;
	irq_handler = r2_irq_handler;

	SDIO->MASK = MASK_WAIT_RESPONSE;
	SDIO->ARG = (uint32_t)rca << 16;
	SDIO->CMD = CMD_LONG_RESP | CMD9_SEND_CSD;
}

/** SELECT/DESELECT_CARD */
void sd::exe_cmd7(uint16_t rca, CSR_Consumer on_done, ErrorCallback on_error) {
	success.csr_consumer = on_done;
	error_callback = on_error;
	irq_handler = r1_irq_handler;

	SDIO->MASK = MASK_WAIT_RESPONSE;
	SDIO->ARG = (uint32_t)rca << 16;
	SDIO->CMD = CMD_SHORT_RESP | CMD7_SELECT_DESELECT_CARD;
}

static void do_exe_acmd6() {
	irq_handler = r1_irq_handler;

	SDIO->MASK = MASK_WAIT_RESPONSE;
	SDIO->ARG = acmd_arg;
	SDIO->CMD = CMD_SHORT_RESP | ACMD6_SET_BUS_WIDTH;
}

/** SET_BUS_WIDTH */
void sd::exe_acmd6(BusWidth width, CSR_Consumer on_done, ErrorCallback on_error) {
	success.csr_consumer = on_done;
	error_callback = on_error;
	acmd_arg = (uint32_t)width;
	exe_acmd = do_exe_acmd6;

	exe_cmd55(0);
}

static void do_exe_acmd42() {
	irq_handler = r1_irq_handler;

	SDIO->MASK = MASK_WAIT_RESPONSE;
	SDIO->ARG = acmd_arg;
	SDIO->CMD = CMD_SHORT_RESP | ACMD42_SET_CLR_CARD_DETECT;
}

/** SET_CLD_CARD_DETECT */
void sd::exe_acmd42(DAT3_PullUp state, CSR_Consumer on_done, ErrorCallback on_error) {
	success.csr_consumer = on_done;
	error_callback = on_error;
	acmd_arg = (uint32_t)state;
	exe_acmd = do_exe_acmd42;

	exe_cmd55(0);
}

void sd::handle_cmd_irq(uint32_t sta) {
	if (sta & SDIO_STA_CTIMEOUT) {
		SDIO->ICR = ICR_CMD_RELATED;
		error_callback(Error::NO_RESPONSE);
		return;
	}
	// ACMD41 expects response R3 which does not contain valid CRC
	if (sta & SDIO_STA_CCRCFAIL && irq_handler != acmd41_irq_handler) {
		SDIO->ICR = ICR_CMD_RELATED;
		error_callback(Error::RESP_CRC_ERROR);
		return;
	}
	if (sta & SDIO_STA_CMDREND || sta & SDIO_STA_CMDSENT && irq_handler == cmd0_irq_handler
			|| sta & SDIO_STA_CCRCFAIL && irq_handler == acmd41_irq_handler)  {
		SDIO->ICR = ICR_CMD_RELATED;
		irq_handler(sta);
		return;
	}
}