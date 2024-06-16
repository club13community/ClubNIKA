//
// Created by independent-variable on 6/16/2024.
//
#include "./source.h"
#include "./source_periph.h"
#include "./charger.h"
#include "./state.h"

namespace supply {
	volatile uint8_t socket_powered;
}

/** Socket is unpowered - switch to battery now. */
static void socket_unpowered_exti_isr();
/** Socket voltage appeared  - debounce before switching to supply from socket */
static void socket_powered_exti_isr();
/** Socket voltage disappeared during debounce. */
static void socket_stable_timer_isr();
/** Socket voltage disappeared during debounce. */
static void socket_unstable_exti_isr();

static void (* volatile exti_isr)();

#define SOCKET_DEBOUNCE_tick	( SOCKET_DEBOUNCE_s * 1'000'000U / TIMER_TICK_us )

void supply::init_source() {
	init_source_pins();
	if (is_socket_powered()) {
		socket_powered = 1U;
		exti_isr = socket_unpowered_exti_isr;
		detect_socket_unpowered();
	} else {
		socket_powered = 0;
		exti_isr = socket_powered_exti_isr;
		detect_socket_powered();
	}
}

void supply::source_exti_isr() {
	exti_isr();
}

void supply::source_timer_isr() {
	socket_stable_timer_isr();
}

static void switch_to_battery() {
	using namespace supply;
	socket_powered = 0;
	__CLREX(); // see charging enabling to understand why
	disable_charging();
	enable_battery();
}

static void switch_to_socket() {
	using namespace supply;
	socket_powered = 1U;
	__CLREX(); // see charging enabling to understand why
	disable_battery();
	if (charging) {
		enable_charging();
	}
}

static void socket_unpowered_exti_isr() {
	using namespace supply;
	switch_to_battery();
	exti_isr = socket_powered_exti_isr;
	detect_socket_powered();
}

static void socket_powered_exti_isr() {
	using namespace supply;
	exti_isr = socket_unstable_exti_isr;
	// no need to set timer's ISR
	start_source_timeout(SOCKET_DEBOUNCE_tick);
	detect_socket_unpowered();
}

static void socket_unstable_exti_isr() {
	using namespace supply;
	stop_source_timeout();
	exti_isr = socket_powered_exti_isr;
	detect_socket_powered();
}

/** Socket voltage is present during all debounce time. */
static void socket_stable_timer_isr() {
	using namespace supply;
	switch_to_socket();
	exti_isr = socket_unpowered_exti_isr;
	detect_socket_unpowered();
	stop_source_timeout(); // not to invoke after timer reloads
}