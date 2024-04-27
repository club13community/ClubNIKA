//
// Created by independent-variable on 4/23/2024.
//

#pragma once
#include <stdint.h>
#include "sd_errors.h"
#include "sd_registers.h"
#include "sd_enums.h"

namespace sd {
	typedef void (* DoneCallback)();
	typedef void (* ErrorCallback)(Error error);
	typedef void (* OCR_Consumer)(OCR_t ocr);
	typedef void (* RCA_Consumer)(uint16_t rca);
	typedef void (* CSR_Consumer)(CSR_t status);

	void exe_cmd0(DoneCallback on_done);
	void exe_cmd8(DoneCallback on_done, ErrorCallback on_error);
	void exe_acmd41(CapacitySupport cs, MinVdd min_vdd, MaxVdd max_vdd, OCR_Consumer on_done, ErrorCallback on_error);
	void exe_cmd2(uint8_t * cid_ptr, DoneCallback on_done, ErrorCallback on_error);
	void exe_cmd3(RCA_Consumer on_done, ErrorCallback on_error);
	void exe_cmd13(uint16_t rca, CSR_Consumer on_done, ErrorCallback on_error);
	void exe_cmd9(uint16_t rca, uint8_t * csd_ptr, DoneCallback on_done, ErrorCallback on_error);
	void exe_cmd7(uint16_t rca, CSR_Consumer on_done, ErrorCallback on_error);
	void exe_acmd6(BusWidth width, CSR_Consumer on_done, ErrorCallback on_error);
	void exe_acmd42(DAT3_PullUp state, CSR_Consumer on_done, ErrorCallback on_error);
	void exe_cmd24(uint32_t addr, CSR_Consumer on_done, ErrorCallback on_error);
	void exe_cmd17(uint32_t addr, CSR_Consumer on_done, ErrorCallback on_error);
	void handle_cmd_irq(uint32_t sta);
}