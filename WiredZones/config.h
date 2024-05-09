//
// Created by independent-variable on 3/30/2024.
//

#pragma once

#define POLARITY	(wired_zones::Measurement::SINKING)
/** Threshold between open/close state */
#define THRESHOLD_mA	10U
/** Hysteresis for open/close state. Actual levels are threshold +/- 0.5 * hysteresis. */
#define HYSTERESIS_mA	5U