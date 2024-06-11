
#include "wired_zones.h"
#include "wired_zones_meas.h"
#include "./periph.h"
#include "periph_allocation.h"
#include "./config.h"
#include "settings.h"

#if ZONES_COUNT != 8
#error Number of wired zones is fixed to 8
#endif

#define INDEX_MASK	0x7U

#define HIGHER_THRESHOLD	(THRESHOLD_mA + (HYSTERESIS_mA + 1) / 2)
#define LOWER_THRESHOLD		(THRESHOLD_mA - (HYSTERESIS_mA + 1) / 2)

#define STACK_SIZE	128U
static TaskHandle_t service;
static StaticTask_t service_ctrl;
static StackType_t service_stack[STACK_SIZE];

static void main_task(void *pvParameters){
	while(1){
		taskYIELD();
	}
}

void wired_zones::init_periph() {
	init_dir_pin();
	init_ch_sel_pins();
	if (POLARITY == Measurement::SINKING) {
		measure_sink_current();
	} else {
		measure_source_current();
	}
}

static volatile uint8_t zone_index;
static volatile ZoneActivation zone_activation;
static volatile uint8_t active_zones;

void wired_zones::start() {
	zone_index = 0;
	active_zones = 0;
	service = xTaskCreateStatic(main_task, "wire sense.", STACK_SIZE, nullptr,
								TASK_NORMAL_PRIORITY, service_stack, &service_ctrl);
}

uint8_t wired_zones::get_active() {
	return active_zones;
}

static inline uint16_t to_mA(uint16_t value_mV) {
	return ((value_mV << 1) + 100) / 200;
}

void wired_zones::process_measurement(uint16_t value) {
	uint16_t current = to_mA(value);
	uint8_t index = zone_index;
	ZoneActivation activation = zone_activation;
	uint8_t mask = 1U << index;
	uint8_t active_now = active_zones;
	if (activation == ZoneActivation::ON_OPEN) {
		// activates by low current
		uint16_t active_threshold = active_now & mask ? HIGHER_THRESHOLD : LOWER_THRESHOLD;
		if (current < active_threshold) {
			active_zones = active_now | mask;
		} else {
			active_zones = active_now & ~mask;
		}
	} if (activation == ZoneActivation::ON_CLOSE) {
		// activates by big current
		uint16_t active_threshold = active_now & mask ? LOWER_THRESHOLD : HIGHER_THRESHOLD;
		if (current > active_threshold) {
			active_zones = active_now | mask;
		} else {
			active_zones = active_now & ~mask;
		}
	}
	zone_index = index + 1U & INDEX_MASK; // check that enabled is done when selecting measurement
}

wired_zones::Measurement wired_zones::select_measurement() {
	ZoneActivationFlags activations = get_zone_activations_for_isr();
	uint8_t enabled = activations.on_close | activations.on_open;
	active_zones &= enabled; // to 'deactivate' zones which were disabled between measurements

	uint8_t index = zone_index;
	ZoneActivation activation = ZoneActivation::NEVER;
	for (uint8_t i = 0; i < 8; i++, index = index + 1 & INDEX_MASK) {
		uint8_t mask = 1U << index;
		if (activations.on_open & mask) {
			activation = ZoneActivation::ON_OPEN;
			break;
		} else if (activations.on_close & mask) {
			activation = ZoneActivation::ON_CLOSE;
			break;
		}
	}
	if (activation == ZoneActivation::NEVER) {
		// all zones disabled - skip measurement
		return Measurement::NONE;
	} else {
		zone_index = index;
		zone_activation = activation;
		select_channel(index);
		return POLARITY;
	}
}



