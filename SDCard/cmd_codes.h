//
// Created by independent-variable on 4/23/2024.
//

#pragma once
#include <stdint.h>

#define CMD0_GO_IDLE_STATE		((uint8_t)0)
#define CMD8_SEND_IF_COND		((uint8_t)8)
#define CMD55_APP_CMD			((uint8_t)55)
#define ACMD41_SD_SEND_OP_COND	((uint8_t)41)
#define CMD2_ALL_SEND_CID		((uint8_t)2)
#define CMD3_SEND_RELATIVE_ADDR	((uint8_t)3)
#define CMD13_SEND_STATUS		((uint8_t)13)
#define CMD9_SEND_CSD			((uint8_t)9)
#define CMD7_SELECT_DESELECT_CARD	((uint8_t)7)
#define ACMD6_SET_BUS_WIDTH			((uint8_t)6)
#define ACMD42_SET_CLR_CARD_DETECT	((uint8_t)42)
#define CMD24_WRITE_BLOCK		((uint8_t)24)
#define CMD17_READ_SINGLE_BLOCK	((uint8_t)17)