//
// Created by independent-variable on 6/16/2024.
//
#include "./fuse.h"
#include "./fuse_periph.h"
#include "./state.h"

#define OVER_CURRENT_PULSE_tick		( OVER_CURRENT_PULSE_ms * 1000U / TIMER_TICK_us )
#define NO_OVER_CURRENT_TIME_tick	( NO_OVER_CURRENT_TIME_s * 1'000'000U / TIMER_TICK_us )

namespace supply {
	volatile uint8_t no_protection;
}

static void (* volatile exti_isr)();
static void (* volatile timer_isr)();

/** Detected over current, starts delay to check if it just short pulse. */
static void over_current_exti_isr();
/** Elapsed delay for short over current pulse. */
static void over_current_pulse_timer_isr();
/** Over current detected during NO_OVER_CURRENT_TIME_s period - apply protection. */
static void repeated_over_current_exti_isr();
/** NO_OVER_CURRENT_TIME_s period elapsed - return to usual over current detection. */
static void no_over_current_timer_isr();
/** Elapsed timeout of protection - try to enable 12V again. */
static void protection_timer_isr();

void supply::init_fuse() {
	init_fuse_pins();
	// enable 12V in several seconds after all system start's
	timer_isr = protection_timer_isr;
	start_fuse_timeout(NO_OVER_CURRENT_TIME_tick);
	no_protection = 1U;
}

void supply::fuse_exti_isr() {
	exti_isr();
}

void supply::fuse_timer_isr() {
	timer_isr();
}

static void enable_protection() {
	using namespace supply;
	no_protection = 0;
	__CLREX(); // see siren enabling to understand why
	disable_12V();
	disable_siren();
}

static void disable_protection() {
	using namespace supply;
	no_protection = 1U;
	__CLREX(); // see siren enabling to understand why
	enable_12V();
	if (siren_on) {
		enable_siren();
	}
}

static void over_current_exti_isr() {
	using namespace supply;
	timer_isr = over_current_pulse_timer_isr;
	start_fuse_timeout(OVER_CURRENT_PULSE_tick);
	ignore_over_current();
}

static void over_current_pulse_timer_isr() {
	using namespace supply;
	if (is_over_current()) {
		// still over current - disable 12V
		enable_protection();
		timer_isr = protection_timer_isr;
		// note: still ignoring "over current" signal
	} else {
		// no over current any longer, check that current is below limits for defined time
		timer_isr = no_over_current_timer_isr;
		exti_isr = repeated_over_current_exti_isr;
		detect_over_current();
	}
	start_fuse_timeout(NO_OVER_CURRENT_TIME_tick);
}

static void repeated_over_current_exti_isr() {
	using namespace supply;
	enable_protection();
	ignore_over_current();
	timer_isr = protection_timer_isr;
	start_fuse_timeout(NO_OVER_CURRENT_TIME_tick);
}

static void no_over_current_timer_isr() {
	using namespace supply;
	stop_fuse_timeout(); // not to be invoked after timer's reload
	exti_isr = over_current_exti_isr;
	// note: "over current" signal is already monitored
}

static void protection_timer_isr() {
	using namespace supply;
	stop_fuse_timeout(); // not to be invoked after timer's reload
	exti_isr = over_current_exti_isr;
	detect_over_current();
	disable_protection();
}