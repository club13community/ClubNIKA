//
// Created by independent-variable on 4/23/2024.
//

#pragma once
#include <stdint.h>
#include <bit>
#include "sd_errors.h"
#include "sd_enums.h"

namespace sd {
	class OCR_t {
	private:
		const uint32_t ocr;
		static constexpr uint32_t vdd_window = (uint32_t)MaxVdd::V3p6;
	public:
		OCR_t(uint32_t ocr) : ocr(ocr) {}

		inline MinVdd min_vdd() {
			// works even if "VDD window" has gaps, like 110101
			uint32_t min_vdd_bit = std::__countr_zero(ocr & vdd_window);
			uint32_t min_vdd_mask = ~((1U << min_vdd_bit) - 1); // all 1 from MSB till min_vdd_bit
			return MinVdd(vdd_window & min_vdd_mask);
		}

		inline MaxVdd max_vdd() {
			// works even if "VDD window" has gaps, like 110101
			uint32_t max_vdd_bit = 31 - std::__countl_zero(ocr & vdd_window);
			uint32_t max_vdd_mask = (1U << (max_vdd_bit + 1)) - 1; // all 1 from max_vdd_bit till LSB
			return MaxVdd(max_vdd_mask & vdd_window);
		}

		inline bool over_2TB() {
			return ocr & 1U << 27;
		}

		inline CapacitySupport capacity_support() {
			return CapacitySupport(ocr & 1U << 30);
		}

		inline bool power_up_done() {
			return ocr & 1U << 31;
		}
	};

	class CSR_t {
	private:
		static constexpr uint32_t ERROR_MASK = 1U << (uint16_t)Error::ARG_OUT_OF_RANGE
				| 1U << (uint16_t)Error::ADDRESS_ERROR | 1U << (uint16_t)Error::BLOCK_LEN_ERROR
				| 1U << (uint16_t)Error::ERASE_SEQ_ERROR | 1U << (uint16_t)Error::WRONG_ERASE_PARAM
				| 1U << (uint16_t)Error::WP_VIOLATION | 1U << (uint16_t)Error::LOCK_UNLOCK_FAILED
				| 1U << (uint16_t)Error::CMD_CRC_ERROR | 1U << (uint16_t)Error::ILLEGAL_COMMAND
				| 1U << (uint16_t)Error::CARD_ECC_FAILED | 1U << (uint16_t)Error::GENERAL_ERROR
				| 1U << (uint16_t)Error::CSD_OVERWRITE | 1U << (uint16_t)Error::AKE_SEQ_ERROR;
	private:
		const uint32_t csr;
	public:
		CSR_t(uint32_t csr) : csr(csr) {}

		inline bool has_errors() {
			return csr & ERROR_MASK;
		}

		inline bool has_error(Error error) {
			return (uint16_t)error < 32 && csr & 1U << (uint16_t)error;
		}

		inline uint32_t error_flags() {
			return csr & ERROR_MASK;
		}

		inline Error any_error() {
			return any_error(csr);
		}

		inline bool card_locked() {
			return csr & 1U << 25;
		}

		/** Prev. command was executed without internal ECC */
		inline bool ecc_used() {
			return !(csr & 1U << 14);
		}

		/** Erase is aborted because of command out of "erase command sequence"*/
		inline bool erase_aborted() {
			return !(csr & 1U << 13);
		}

		/** Some address spaces were not erased, because they are write-protected */
		inline bool erased_partially() {
			return csr & 1U << 15;
		}

		inline State state() {
			return State(csr & 0xFU << 9);
		}

		inline bool ready_for_data() {
			return csr & 1U << 8;
		}

		inline bool app_cmd() {
			return csr & 1U << 5;
		}

		static inline Error any_error(uint32_t error_flags) {
			if (error_flags & 1U << (uint16_t)Error::ARG_OUT_OF_RANGE) {
				return Error::ARG_OUT_OF_RANGE;
			}
			if (error_flags & 1U << (uint16_t)Error::ADDRESS_ERROR) {
				return Error::ADDRESS_ERROR;
			}
			if (error_flags & 1U << (uint16_t)Error::BLOCK_LEN_ERROR) {
				return Error::BLOCK_LEN_ERROR;
			}
			if (error_flags & 1U << (uint16_t)Error::ERASE_SEQ_ERROR) {
				return Error::ERASE_SEQ_ERROR;
			}
			if (error_flags & 1U << (uint16_t)Error::WRONG_ERASE_PARAM) {
				return Error::WRONG_ERASE_PARAM;
			}
			if (error_flags & 1U << (uint16_t)Error::WP_VIOLATION) {
				return Error::WP_VIOLATION;
			}
			if (error_flags & 1U << (uint16_t)Error::LOCK_UNLOCK_FAILED) {
				return Error::LOCK_UNLOCK_FAILED;
			}
			if (error_flags & 1U << (uint16_t)Error::CMD_CRC_ERROR) {
				return Error::CMD_CRC_ERROR;
			}
			if (error_flags & 1U << (uint16_t)Error::ILLEGAL_COMMAND) {
				return Error::ILLEGAL_COMMAND;
			}
			if (error_flags & 1U << (uint16_t)Error::CARD_ECC_FAILED) {
				return Error::CARD_ECC_FAILED;
			}
			if (error_flags & 1U << (uint16_t)Error::GENERAL_ERROR) {
				return Error::GENERAL_ERROR;
			}
			if (error_flags & 1U << (uint16_t)Error::CSD_OVERWRITE) {
				return Error::CSD_OVERWRITE;
			}
			if (error_flags & 1U << (uint16_t)Error::AKE_SEQ_ERROR) {
				return Error::AKE_SEQ_ERROR;
			}
			return NO_ERROR;
		}
	};
}