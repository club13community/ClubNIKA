//
// Created by independent-variable on 4/21/2024.
//

#pragma once
#include "sd_types.h"

#define MIN_VDD	(sd::MinVdd::V3p1)
#define MAX_VDD	(sd::MaxVdd::V3p5)

/** Approximate freq. Real freq. will be <= than this. */
#define CLK_APP_FREQ	((uint32_t)2'000'000)
#define CLK_INIT_FREQ	((uint32_t)400'000)
/** Time for card to 1) start sending requested block; 2) finish programming of received block */
#define READ_WRITE_TIME_ms	200

#define PRESENCE_CHECK_PERIOD_ms		20
/** How many consequent checks, that card is inserted, should pass before card is assumed inserted */
#define INSERTION_DEBOUNCE_SAMPLES		50
/** How many consequent checks, that card is removed, should pass before card is assumed removed. */
#define REMOVAL_DEBOUNCE_SAMPLES		50