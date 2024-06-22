//
// Created by independent-variable on 4/23/2024.
//

#pragma once
#include <stdint.h>

namespace sd {
	enum class Error : uint16_t {
		/** Invalid argument of command */
		ARG_OUT_OF_RANGE = 31,
		/** Address is not block length aligned */
		ADDRESS_ERROR = 30,
		BLOCK_LEN_ERROR = 29,
		/** Wrong sequence of erase commands */
		ERASE_SEQ_ERROR = 28,
		/** Invalid selection of a block to erase */
		WRONG_ERASE_PARAM = 27,
		/** Attempted to write to protected area */
		WP_VIOLATION = 26,
		/** Wrong passw. or command sequence to lock/unlock */
		LOCK_UNLOCK_FAILED = 24,
		/** Failed CRC for command */
		CMD_CRC_ERROR = 23,
		ILLEGAL_COMMAND = 22,
		CARD_ECC_FAILED = 21,
		CARD_CONTROLLER_ERROR = 20,
		/** General or unknown error */
		GENERAL_ERROR = 19,
		CSD_OVERWRITE = 16,
		/** Error in the sequence of authentication process */
		AKE_SEQ_ERROR = 3,
		/** Response to a command was not received */
		NO_RESPONSE = 32,
		RESP_CRC_ERROR = 33,
		NOT_SUPPORTED_VDD = 34,
		NOT_SUPPORTED_CARD = 35,
		/** FIFO under/over ran */
		FIFO_ERROR = 36,
		/** Data was not received or programmed in time */
		DATA_TIMEOUT = 37,
		DATA_CRC_ERROR = 38,
		/** New operation was initiated while prev. is still running */
		CONCURRENT_ACCESS = 39,
		/** Card is not inserted */
		NO_CARD = 40,
		/** No error, all ok */
		NONE = 0xFFFF
	};

	/** @returns true if response to command was received and valid so error is taken from response */
	inline bool is_from_response(Error error) {
		return (uint16_t)error <= 31 || error == Error::NONE;
	}
}
