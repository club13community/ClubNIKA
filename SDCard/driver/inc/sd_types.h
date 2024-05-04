//
// Created by independent-variable on 4/25/2024.
//

#pragma once
#include <stdint.h>

namespace sd {
	enum class MinVdd : uint32_t {
		V3p5 = 1 << 23, V3p4 = (1 << 22) | V3p5, V3p3 = (1 << 21) | V3p4,
		V3p2 = (1 << 20) | V3p4, V3p1 = (1 << 19) | V3p2, V3p0 = (1 << 18) | V3p1, V2p9 = (1 << 17) | V3p0,
		V2p8 = (1 << 16) | V2p9, V2p7 = (1 << 15) | V2p8
	};
	enum class MaxVdd : uint32_t {
		V2p8 = 1 << 15, V2p9 = (1 << 16) | V2p8, V3p0 = (1 << 17) | V2p9,
		V3p1 = (1 << 18) | V3p0, V3p2 = (1 << 19) | V3p1, V3p3 = (1 << 20) | V3p2, V3p4 = (1 << 21) | V3p3,
		V3p5 = (1 << 22) | V3p4, V3p6 = (1 << 23) | V3p5
	};
	enum class CapacitySupport : uint32_t {
		SC = 0, HC_XC = 1 << 30
	};
	enum class State : uint32_t {
		IDLE = 0 << 9, READY = 1U << 9, IDENT = 2U << 9, STBY = 3U << 9, TRAN = 4U << 9,
		DATA = 5U << 9, RCV = 6U << 9, PRG = 7U << 9, DIS = 8U << 9
	};
	enum class DAT3_PullUp : uint32_t {
		ENABLE = 0, DISABLE = 1
	};
	enum class BusWidth : uint32_t {
		ONE_BIT = 0, FOUR_BITS = 2
	};
}