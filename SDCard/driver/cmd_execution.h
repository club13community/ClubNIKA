//
// Created by independent-variable on 4/23/2024.
//

#pragma once
#include <stdint.h>
#include "sd_errors.h"
#include "./card_registers.h"
#include "sd_types.h"
#include "sd_callbacks.h"

namespace sd {
	void exe_cmd0(Callback on_done);
	void exe_cmd8(Callback on_done);
	void exe_acmd41(CapacitySupport cs, MinVdd min_vdd, MaxVdd max_vdd, OCR_Consumer callback);
	void exe_cmd2(uint8_t * cid_ptr, Callback callback);
	void exe_cmd3(RCA_Consumer callback);
	void exe_cmd13(uint16_t rca, CSR_Consumer callback);
	void exe_cmd9(uint16_t rca, uint8_t * csd_ptr, Callback callback);
	void exe_cmd7(uint16_t rca, CSR_Consumer callback);
	void exe_acmd6(BusWidth width, CSR_Consumer callback);
	void exe_acmd42(DAT3_PullUp state, CSR_Consumer callback);
	void exe_cmd24(uint32_t addr, CSR_Consumer callback);
	void exe_cmd17(uint32_t addr, CSR_Consumer callback);
	void exe_cmd25(uint32_t addr, CSR_Consumer callback);
	void exe_cmd18(uint32_t addr, CSR_Consumer callback);
	void pend_exe_cmd12(CSR_Consumer callback);
	void handle_cmd_irq(uint32_t sta);
}