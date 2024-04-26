//
// Created by independent-variable on 4/26/2024.
//
#include "sd_info.h"
#include "sd_info_private.h"
#include "sd_enums.h"

namespace sd {
	volatile CapacitySupport hcs;
	volatile uint16_t RCA;
	volatile uint32_t serial_number;
	volatile uint32_t capacity_kb;
	volatile bool write_protected;
}

void sd::parse_CID(uint8_t * buff) {
	serial_number;
}

uint32_t sd::get_capacity_kb() {
	return capacity_kb;
}

/** @returns integral number of MBytes */
uint32_t sd::get_capacity_mb() {
	return capacity_kb >> 10;
}

/** @returns integral number of GBytes */
uint32_t sd::get_capacity_gb() {
	return capacity_kb >> 20;
}

bool sd::is_write_protected() {
	return write_protected;
}

static bool parse_CSD_v1(uint8_t * buff) {
	// capacity = (2^READ_BL_LEN) * (C_SIZE + 1) * (2^[C_SIZE_MULT + 2])
	// READ_BL_LEN: [83:80] - 4 bits, < 12
	uint8_t READ_BL_LEN = buff[10] & 0xF;
	// C_SIZE: [73:62] - 12 bits
	uint8_t C_SIZE_63_62 = buff[7] /* 63..56*/ >> 6;
	uint8_t C_SIZE_71_64 = buff[8];
	uint8_t CSIZE_73_72 = buff[9] /*79..72*/ & 0x3;
	uint16_t C_SIZE = (uint16_t)CSIZE_73_72 << 10 | (uint16_t)C_SIZE_71_64 << 2 | (uint16_t)C_SIZE_63_62;
	// C_SIZE_MULT: [49:47] - 3 bits, < 8
	uint8_t C_SIZE_MULT_47 = buff[5] /*47..40*/ >> 7;
	uint8_t C_SIZE_MULT_49_48 = buff[6] /*55..48*/ & 0x3;
	uint8_t C_SIZE_MULT = C_SIZE_MULT_49_48 << 1 | C_SIZE_MULT_47;
	uint32_t capacity_bytes = ((uint32_t)1 << (READ_BL_LEN + C_SIZE_MULT + 2) * ((uint32_t)C_SIZE + 1));
	if (capacity_bytes < 1024 || (capacity_bytes & 0x3FF) != 0) {
		// too small capacity or is not integral number of kBytes
		return false;
	}
	sd::capacity_kb = capacity_bytes >> 10;
	// write protected = PERM_WRITE_PROTECT(13) | TMP_WRITE_PROTECT(12) | WP_UPC(9)
	sd::write_protected = *(uint16_t *)buff & (uint16_t)1 << 13 & (uint16_t)1 << 12 & (uint16_t)1 << 9;
	return true;
}

static bool parse_CSD_v2(uint8_t * buff) {
	// HC(<=32GB) or XC(<=2TB) card
	// capacity = 512KB * (C_SIZE + 1)
	// C_SIZE: [69:48] - 22 bits
	uint32_t C_SIZE = *(uint32_t *)(buff + 6) & 0x3FFFFF;
	sd::capacity_kb = (C_SIZE + 1) << 9;
	// write protected = PERM_WRITE_PROTECT(13) | TMP_WRITE_PROTECT(12) | WP_UPC(9)
	sd::write_protected = *(uint16_t *)buff & (uint16_t)1 << 13 & (uint16_t)1 << 12 & (uint16_t)1 << 9;
	return true;
}

/** @returns true if card is supported by this driver */
bool sd::parse_CSD(uint8_t * buff) {
	// CSD_STRUCTURE: [127:126] - 2 bits
	uint8_t CSD_STRUCTURE = buff[15] /*127..120*/ >> 6;
	switch (CSD_STRUCTURE) {
		case 0: return parse_CSD_v1(buff);
		case 1: return parse_CSD_v2(buff);
		default: return false; // UC card or wrong value
	}
}
