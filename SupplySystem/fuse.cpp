//
// Created by independent-variable on 6/16/2024.
//
#include "./fuse.h"
#include "./fuse_periph.h"
#include "./outputs.h"

#define OVER_CURRENT_PULSE_tick		( OVER_CURRENT_PULSE_ms * 1000U / TIMER_TICK_us )
#define NO_OVER_CURRENT_TIME_tick	( NO_OVER_CURRENT_TIME_s * 1'000'000U / TIMER_TICK_us )

static void (* volatile on_exti)();
static void (* volatile on_timer)();

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
	on_timer = protection_timer_isr;
	start_fuse_timeout(NO_OVER_CURRENT_TIME_tick);
}

void supply::fuse_exti_isr() {
	on_exti();
}

void supply::fuse_timer_isr() {
	on_timer();
}

static inline void enable_protection() {
	using namespace supply;
	output_protection_changed(true);
}

static inline void disable_protection() {
	using namespace supply;
	output_protection_changed(false);
}

static void over_current_exti_isr() {
	using namespace supply;
	on_timer = over_current_pulse_timer_isr;
	start_fuse_timeout(OVER_CURRENT_PULSE_tick);
	ignore_over_current();
}

static void over_current_pulse_timer_isr() {
	using namespace supply;
	if (is_over_current()) {
		// still over current - disable 12V
		enable_protection();
		on_timer = protection_timer_isr;
		// note: still ignoring "over current" signal
	} else {
		// no over current any longer, check that current is below limits for defined time
		on_timer = no_over_current_timer_isr;
		on_exti = repeated_over_current_exti_isr;
		detect_over_current();
	}
	start_fuse_timeout(NO_OVER_CURRENT_TIME_tick);
}

static void repeated_over_current_exti_isr() {
	using namespace supply;
	enable_protection();
	ignore_over_current();
	on_timer = protection_timer_isr;
	start_fuse_timeout(NO_OVER_CURRENT_TIME_tick);
}

static void no_over_current_timer_isr() {
	using namespace supply;
	stop_fuse_timeout(); // not to be invoked after timer's reload
	on_exti = over_current_exti_isr;
	// note: "over current" signal is already monitored
}

static void protection_timer_isr() {
	using namespace supply;
	stop_fuse_timeout(); // not to be invoked after timer's reload
	on_exti = over_current_exti_isr;
	detect_over_current();
	disable_protection();
}