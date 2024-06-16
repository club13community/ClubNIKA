//
// Created by independent-variable on 6/16/2024.
//
#include "./periph.h"
#include "./source.h"
#include "./charger.h"
#include "./fuse.h"
#include "./state.h"

namespace supply {
	volatile bool siren_on;
}

void init() {
	using namespace supply;

	init_12V_switches();
	init_timer();
	init_exti_lines();
	init_source();
	init_fuse();
	init_charger();

	siren_on = false;
}

void start() {
	using namespace supply;
	start_timer();
	enable_exti_lines();
}

void turn_siren_on() {
	using namespace supply;
	siren_on = true;
	enable_siren_if(&no_protection);
}

void turn_siren_off() {
	using namespace supply;
	siren_on = false;
	disable_siren();
}

void exti_isr() {
	using namespace supply;
	if (is_source_exti()) {
		source_exti_isr();
	}
	if (is_fuse_exti()) {
		fuse_exti_isr();
	}
}

void timer_isr() {
	using namespace supply;
	if (is_source_timeout()) {
		source_timer_isr();
	}
	if (is_fuse_timeout()) {
		fuse_timer_isr();
	}
}
