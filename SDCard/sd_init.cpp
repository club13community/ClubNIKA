//
// Created by independent-variable on 4/21/2024.
//
#include "sd.h"
#include "stm32f10x.h"
#include "periph.h"
#include "timing.h"
#include "cmd_execution.h"
#include "sd_errors.h"
#include "config.h"
#include "sd_info_private.h"

void sd::init_periph() {
	init_sdio();
	init_sdio_pins();
	init_detect_pin();
	init_dma();
}

using namespace sd;
static uint8_t cid_cds_buff[16];

#define MAX_ATTEMPTS	2
/** 1ms between attempts, ACMD41 will be executed not longer than 1 sec(req. by spec) */
#define MAX_ACMD41_ATTEMPTS	1000
static volatile uint16_t attempts;
static void (* callback)(Error);

#define IS_RETRYABLE(error) ( (bool)(error == Error::CMD_CRC_ERROR || error == Error::RESP_CRC_ERROR || error == Error::GENERAL_ERROR) )

static void cmd0_done();
static void cmd8_done();
static void cmd8_failed(Error error);
static void exe_acmd41_in_1ms();
static void acmd41_done(OCR_t ocr);
static void acmd41_failed(Error error);
static void cmd2_done();
static void cmd2_failed(Error error);
static void cmd3_done(uint16_t rca);
static void cmd3_failed(Error error);
static void cmd9_done();
static void cmd9_failed(Error error);
static void cmd7_done(CSR_t csr);
static void cmd7_failed(Error error);
static void acmd42_done(CSR_t csr);
static void acmd42_failed(Error error);
static void acmd6_done(CSR_t csr);
static void acmd6_failed(Error error);
static void init_done();
static void init_failed(Error error);

void sd::init_card(void (* callback)(Error)) {
	::callback = callback;
	uint32_t freq = set_slow_clk() - 1;
	if (freq == 0) {
		throw std::exception();
	}
	uint32_t x74_clk_delay_250u = ((uint32_t)74000 * 4) / freq;
	if (x74_clk_delay_250u > timing::CoarseTimer::MAX_DELAY_250us) {
		throw std::exception();
	}
	uint16_t pwr_up_delay_250u = x74_clk_delay_250u > 4 ? x74_clk_delay_250u : 4;
	en_sdio_clk();
	constexpr timing::Callback invoke_cmd0 = []() {
		exe_cmd0(cmd0_done);
	};
	TIMER.invoke_in_ticks(pwr_up_delay_250u, invoke_cmd0);
}

/** Card reset, now it is in 'idle' state */
static void cmd0_done() {
	// send interface conditions
	attempts = MAX_ATTEMPTS;
	exe_cmd8(cmd8_done, cmd8_failed);
}

/** Interface conditions sent */
static void cmd8_done() {
	// card is HC or XC
	hcs = CapacitySupport::HC_XC;
	// send operational conditions
	attempts = MAX_ACMD41_ATTEMPTS;
	exe_acmd41(CapacitySupport::HC_XC, MIN_VDD, MAX_VDD, acmd41_done, acmd41_failed);
};

static void cmd8_failed(Error error) {
	bool has_attempts = --attempts > 0;
	if (error == Error::NO_RESPONSE) {
		// card is SC
		hcs = CapacitySupport::SC;
		// send operational conditions
		attempts = MAX_ACMD41_ATTEMPTS;
		exe_acmd41(CapacitySupport::SC, MIN_VDD, MAX_VDD, acmd41_done, acmd41_failed);
	} else if (has_attempts && IS_RETRYABLE(error)) {
		exe_cmd8(cmd8_done, cmd8_failed);
	} else {
		init_failed(error);
	}
}

/** Send operational cond. in 1ms */
static void exe_acmd41_in_1ms() {
	constexpr timing::Callback invoke_acmd41 = []() {
		exe_acmd41(hcs, MIN_VDD, MAX_VDD, acmd41_done, acmd41_failed);
	};
	TIMER.invoke_in_ms(1, invoke_acmd41);
}

/** Operational cond. sent */
static void acmd41_done(OCR_t ocr) {
	bool has_attempts = --attempts > 0;
	if (ocr.power_up_done()) {
		// request CID
		attempts = MAX_ATTEMPTS;
		exe_cmd2(cid_cds_buff, cmd2_done, cmd2_failed);
	} else if (has_attempts) {
		exe_acmd41_in_1ms();
	} else {
		init_failed(Error::NOT_SUPPORTED_CARD);
	}
}

static void acmd41_failed(Error error) {
	bool has_attempts = --attempts > 0;
	if (has_attempts && IS_RETRYABLE(error)) {
		exe_acmd41_in_1ms();
	} else {
		init_failed(error);
	}
}

/** CID received */
static void cmd2_done() {
	parse_CID(cid_cds_buff);
	// request RCA
	attempts = MAX_ATTEMPTS;
	exe_cmd3(cmd3_done, cmd3_failed);
}

static void cmd2_failed(Error error) {
	bool has_attempts = --attempts > 0;
	if (has_attempts && IS_RETRYABLE(error)) {
		exe_cmd2(cid_cds_buff, cmd2_done, cmd2_failed);
	} else {
		init_failed(error);
	}
}

/** RCA received */
static void cmd3_done(uint16_t rca) {
	RCA = rca;
	// request card specific data
	attempts = MAX_ATTEMPTS;
	exe_cmd9(rca, cid_cds_buff, cmd9_done, cmd9_failed);
}

static void cmd3_failed(Error error) {
	bool has_attempts = --attempts > 0;
	if (has_attempts && IS_RETRYABLE(error)) {
		exe_cmd3(cmd3_done, cmd3_failed);
	} else {
		init_failed(error);
	}
}

/** Card specific data received */
static void cmd9_done() {
	if (!parse_CSD(cid_cds_buff)) {
		// not supported card
		init_failed(Error::NOT_SUPPORTED_CARD);
	} else {
		// select card(to disable pull-up on DAT3 and set bus width)
		attempts = MAX_ATTEMPTS;
		exe_cmd7(RCA, cmd7_done, cmd7_failed);
	}
}

static void cmd9_failed(Error error) {
	bool has_attempts = --attempts > 0;
	if (has_attempts && IS_RETRYABLE(error)) {
		exe_cmd9(RCA, cid_cds_buff, cmd9_done, cmd9_failed);
	} else {
		init_failed(error);
	}
}

/** Card selected, now it is in 'tran' state */
static void cmd7_done(CSR_t csr) {
	// disable pull-up on DAT3
	attempts = MAX_ATTEMPTS;
	exe_acmd42(DAT3_PullUp::DISABLE, acmd42_done, acmd42_failed);
}

static void cmd7_failed(Error error) {
	bool has_attempts = --attempts > 0;
	if (has_attempts && IS_RETRYABLE(error)) {
		exe_cmd7(RCA, cmd7_done, cmd7_failed);
	} else {
		init_failed(error);
	}
}

/** Pull-up on DAT3 is disabled */
static void acmd42_done(CSR_t csr) {
	// change bus width
	attempts = MAX_ATTEMPTS;
	exe_acmd6(BusWidth::FOUR_BITS, acmd6_done, acmd6_failed);
}

static void acmd42_failed(Error error) {
	bool has_attempts = --attempts > 0;
	if (has_attempts && IS_RETRYABLE(error)) {
		exe_acmd42(DAT3_PullUp::DISABLE, acmd42_done, acmd42_failed);
	} else {
		init_failed(error);
	}
}

/** Bus width changed */
static void acmd6_done(CSR_t csr) {
	use_4bits_dat();
	init_done();
}

static void acmd6_failed(Error error) {
	bool has_attempts = --attempts > 0;
	if (has_attempts && IS_RETRYABLE(error)) {
		exe_acmd6(BusWidth::FOUR_BITS, acmd6_done, acmd6_failed);
	} else {
		init_failed(error);
	}
}

static void init_done() {
	set_fast_clk();
	callback(NO_ERROR);
}

static void init_failed(Error error) {
	dis_sdio_clk();
	callback(error);
}