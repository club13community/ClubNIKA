//
// Created by independent-variable on 6/16/2024.
//
#include "./charger.h"
#include "./charger_periph.h"

static volatile bool charging;
static volatile uint8_t battery_source;

void supply::init_charger(Source source) {
	init_charger_pins();
	charging = false;
	battery_source = source == Source::BATTERY ? 1U : 0;
}

void supply::charger_source_changed(Source new_source) {
	if (new_source == Source::BATTERY) {
		battery_source = 1U;
		__CLREX();
		disable_charging();
	} else {
		battery_source = 0U;
		__CLREX();
		if (charging) {
			enable_charging();
		}
	}
}

static void safe_enable_charging() {
	using namespace supply;
	do {
		uint8_t dont_en = __LDREXB(const_cast<uint8_t *>(&battery_source));
		if (dont_en) {
			__CLREX();
			return;
		}
	} while (enable_charging_excl());
}