//
// Created by independent-variable on 4/28/2024.
//

#pragma once
#include "sd_errors.h"
#include "sd_registers.h"

namespace sd {
	typedef void (* Callback)(Error error);
	typedef void (* OCR_Consumer)(OCR_t ocr, Error error);
	typedef void (* RCA_Consumer)(uint16_t rca, CSR_t csr, Error error);
	typedef void (* CSR_Consumer)(CSR_t status, Error error);
	typedef void (* DataCallback)(uint32_t block_count, Error error);
}