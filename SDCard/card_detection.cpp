//
// Created by independent-variable on 5/1/2024.
//
#include "card_detection.h"
#include "periph.h"

using namespace sd;

static void (* volatile callback)();
static volatile uint8_t debounce_samples;

static void do_wait_insertion();
static void debounce_insertion();

void sd::wait_insertion(void (* on_inserted)()) {
	callback = on_inserted;
	do_wait_insertion();
}

static void do_wait_insertion() {
	if (is_card_inserted()) {
		debounce_samples = INSERTION_DEBOUNCE_SAMPLES;
		TIMER.invoke_in_ms(PRESENCE_CHECK_PERIOD_ms, debounce_insertion);
	} else {
		TIMER.invoke_in_ms(PRESENCE_CHECK_PERIOD_ms, do_wait_insertion);
	}
}

static void debounce_insertion() {
	if (is_card_inserted()) {
		if (--debounce_samples == 0) {
			// card is assumed inserted
			callback();
		} else {
			// continue debouncing
			TIMER.invoke_in_ms(PRESENCE_CHECK_PERIOD_ms, debounce_insertion);
		}
	} else {
		// did not pass debounce
		TIMER.invoke_in_ms(PRESENCE_CHECK_PERIOD_ms, do_wait_insertion);
	}
}

static void do_check_presence();

void sd::check_presence(void (* on_absent)()) {
	callback = on_absent;
	TIMER.every_ms_invoke(PRESENCE_CHECK_PERIOD_ms, do_check_presence);
}

static void do_check_presence() {
	if (!is_card_inserted()) {
		callback();
	}
}

static void do_wait_removal();
static void debounce_absence();

void sd::wait_removal(void (* on_removed)()) {
	callback = on_removed;
	do_wait_removal();
}

static void do_wait_removal() {
	if (!is_card_inserted()) {
		debounce_samples = REMOVAL_DEBOUNCE_SAMPLES;
		TIMER.invoke_in_ms(PRESENCE_CHECK_PERIOD_ms, debounce_absence);
	} else {
		TIMER.invoke_in_ms(PRESENCE_CHECK_PERIOD_ms, do_wait_removal);
	}
}

static void debounce_absence() {
	if (!is_card_inserted()) {
		if (--debounce_samples == 0) {
			// card is removed
			callback();
		} else {
			// continue debouncing
			TIMER.invoke_in_ms(PRESENCE_CHECK_PERIOD_ms, debounce_absence);
		}
	} else {
		// didn't pass debounce
		TIMER.invoke_in_ms(PRESENCE_CHECK_PERIOD_ms, do_wait_removal);
	}
}