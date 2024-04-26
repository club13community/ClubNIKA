//
// Created by independent-variable on 4/21/2024.
//

#pragma once
#include "sd_enums.h"

#define MIN_VDD	(sd::MinVdd::V3p1)
#define MAX_VDD	(sd::MaxVdd::V3p5)

/** Approximate freq. Real freq. will be <= than this. */
#define CLK_APP_FREQ		((uint32_t)6000000)
#define CLK_INIT_FREQ	((uint32_t)400000)