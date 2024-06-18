//
// Created by independent-variable on 6/16/2024.
//
#include "./SupplySystem.h"
#include "supply_system_meas.h"
#include "./periph.h"
#include "./source.h"
#include "./charger.h"
#include "./fuse.h"
#include "./outputs.h"

static volatile uint16_t battery_mV;

void supply::init() {
	init_timer();
	init_exti_lines();

	Source ini_src = init_source();
	init_charger(ini_src);
	init_fuse();
	init_outputs();

	start_timer();
	enable_exti_lines();

	battery_mV = 12'000U; // typical value in case requested before first measurement
}

void supply::start() {
	// todo create thread or remove method
}

void supply::exti_isr() {
	using namespace supply;
	if (is_source_exti()) {
		source_exti_isr();
	}
	if (is_fuse_exti()) {
		fuse_exti_isr();
	}
}

void supply::timer_isr() {
	using namespace supply;
	if (is_source_timeout()) {
		source_timer_isr();
	}
	if (is_fuse_timeout()) {
		fuse_timer_isr();
	}
}

void supply::process_battery_measurement(uint16_t value) {
	battery_mV = value * 6 - BATTERY_OFFSET_mV; // battery voltage in mV
}

uint16_t supply::get_battery_mV() {
	return battery_mV;
}

static constexpr uint8_t mV_to_pct_size = 12;
static const uint16_t mV_to_pct[mV_to_pct_size][2] = {
		{12'850, 100},
		{12'800, 99},
		{12'750, 90},
		{12'500, 80},
		{12'300, 70},
		{12'150, 60},
		{12'050, 50},
		{11'950, 40},
		{11'810, 30},
		{11'660, 20},
		{11'510, 10},
		{10'500, 0}
};

uint8_t supply::get_battery_pct() {
	uint16_t bat_mV = battery_mV;
	for (uint8_t i = 0; i < mV_to_pct_size; i++) {
		if (bat_mV >= mV_to_pct[i][0]) {
			return mV_to_pct[i][1];
		}
	}
	return 0;
}