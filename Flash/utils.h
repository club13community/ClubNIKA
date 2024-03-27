//
// Created by independent-variable on 3/27/2024.
//

#pragma once
#include <stdint.h>
#include "flash.h"

namespace flash {
	/*
	 * Address bits: 1bit - any, 13bits - page, 10bits - byte
	 * are split on bytes: 0 + page[12..6], page[5..0] + byte[9..8], byte[7..0]
	 */
	inline uint8_t address_byte_1(PageAddress page, ByteOffset byte) {
		return page >> 6;
	}

	inline uint8_t address_byte_2(PageAddress page, ByteOffset byte) {
		return ((page & 0x3FU) << 2) | (byte >> 8);
	}

	inline uint8_t address_byte_3(PageAddress page, ByteOffset byte) {
		return byte;
	}

	inline void put_address_bytes(PageAddress page, ByteOffset byte, volatile uint8_t * dest) {
		dest[0] = address_byte_1(page, byte);
		dest[1] = address_byte_2(page, byte);
		dest[2] = address_byte_3(page, byte);
	}
}
