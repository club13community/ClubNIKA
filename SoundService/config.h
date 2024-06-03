//
// Created by independent-variable on 5/31/2024.
//

#pragma once

/** Gap between converter output and ground(HW limitation). Used to shift 0-level of samples. */
#define MIN_VOUT_TO_GND_mV	200U
/** Gap between analog supply voltage and converter output(HW limitation). Used to shift 0-level of samples. */
#define MIN_VDD_TO_VOUT_mv	1'300U
/** Min. analog voltage for converter. Used to shift 0-level of samples. */
#define MIN_VDD_mv			3'000U
/* Allowed signal amplitude in wav file is (255 - MIN_VOUT_TO_GND_mV / min_lsb - MIN_VDD_TO_VOUT_mv / min_lsb) / 2,
 * where min_lsb = MIN_VDD_mv / 256.
 * With current config it is 25%. */