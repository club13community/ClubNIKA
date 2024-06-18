//
// Created by independent-variable on 6/16/2024.
//
#include "./source.h"
#include "./source_periph.h"
#include "./charger.h"

/** Socket is unpowered - switch to battery now. */
static void socket_unpowered_exti_isr();
/** Socket voltage appeared  - debounce before switching to supply from socket */
static void socket_powered_exti_isr();
/** Socket voltage disappeared during debounce. */
static void socket_stable_timer_isr();
/** Socket voltage disappeared during debounce. */
static void socket_unstable_exti_isr();

static volatile supply::Source source;
static void (* volatile on_exti)();

#define SOCKET_DEBOUNCE_tick	( SOCKET_DEBOUNCE_s * 1'000'000U / TIMER_TICK_us )

supply::Source supply::init_source() {
	init_source_pins();
	Source initial;
	if (is_socket_powered()) {
		initial = Source::SOCKET;
		on_exti = socket_unpowered_exti_isr;
		detect_socket_unpowered();
	} else {
		initial = Source::BATTERY;
		on_exti = socket_powered_exti_isr;
		detect_socket_powered();
	}
	source = initial;
	return initial;
}

supply::Source supply::get_source() {
	return source;
}

void supply::source_exti_isr() {
	on_exti();
}

void supply::source_timer_isr() {
	socket_stable_timer_isr();
}

static void switch_to_battery() {
	using namespace supply;
	source = Source::BATTERY;
	charger_source_changed(Source::BATTERY);
	enable_battery();
}

static void switch_to_socket() {
	using namespace supply;
	source = Source::SOCKET;
	disable_battery();
	charger_source_changed(Source::SOCKET);
}

static void socket_unpowered_exti_isr() {
	using namespace supply;
	switch_to_battery();
	on_exti = socket_powered_exti_isr;
	detect_socket_powered();
}

static void socket_powered_exti_isr() {
	using namespace supply;
	on_exti = socket_unstable_exti_isr;
	// no need to set timer's ISR
	start_source_timeout(SOCKET_DEBOUNCE_tick);
	detect_socket_unpowered();
}

static void socket_unstable_exti_isr() {
	using namespace supply;
	stop_source_timeout();
	on_exti = socket_powered_exti_isr;
	detect_socket_powered();
}

/** Socket voltage is present during all debounce time. */
static void socket_stable_timer_isr() {
	using namespace supply;
	switch_to_socket();
	on_exti = socket_unpowered_exti_isr;
	detect_socket_unpowered();
	stop_source_timeout(); // not to invoke after timer reloads
}