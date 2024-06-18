//
// Created by independent-variable on 6/17/2024.
//
#include "SupplySystem.h"
#include "./outputs.h"
#include "./outputs_periph.h"

static volatile bool siren_on;
static volatile uint8_t protection_on;

void supply::init_outputs() {
	init_output_switches();
	protection_on = 1U;
	siren_on = false;
}

void supply::output_protection_changed(bool applied) {
	if (applied) {
		protection_on = 1U;
		__CLREX();
		disable_12V();
		disable_siren();
	} else {
		protection_on = 0;
		__CLREX();
		enable_12V();
		if (siren_on) {
			enable_siren();
		}
	}
}

static inline void safe_enable_siren() {
	using namespace supply;
	do {
		uint8_t dont_en = __LDREXB(const_cast<uint8_t *>(&protection_on));
		if (dont_en) {
			__CLREX();
			return;
		}
	} while (enable_siren_excl());
}

void supply::turn_on_siren() {
	siren_on = true;
	safe_enable_siren();
}

void supply::turn_off_siren() {
	siren_on = false;
	disable_siren();
}