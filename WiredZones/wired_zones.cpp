
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

static const ZoneActivation * activations;
static volatile uint8_t measured_zone;
static volatile uint8_t active_zones;

void wired_zones::start() {
	measured_zone = 0;
	active_zones = 0;
	activations = get_zone_activations();
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
	uint8_t index = measured_zone;
	uint8_t active_now = active_zones;
	uint8_t zone_mask = 1U << index;
	ZoneActivation activation = activations[index];
	if (activation == ZoneActivation::ON_OPEN) {
		// activates by low current
		uint16_t active_threshold = active_now & zone_mask ? HIGHER_THRESHOLD : LOWER_THRESHOLD;
		if (current < active_threshold) {
			active_zones = active_now | zone_mask;
		} else {
			active_zones = active_now & ~zone_mask;
		}
	} if (activation == ZoneActivation::ON_CLOSE) {
		// activates by big current
		uint16_t active_threshold = active_now & zone_mask ? LOWER_THRESHOLD : HIGHER_THRESHOLD;
		if (current > active_threshold) {
			active_zones = active_now | zone_mask;
		} else {
			active_zones = active_now & ~zone_mask;
		}
	}
}

wired_zones::Measurement wired_zones::select_measurement() {
	uint8_t zone = measured_zone + 1 & INDEX_MASK;
	bool skip = true;
	for (uint8_t i = 0; i < 8; i++, zone = zone + 1 & INDEX_MASK) {
		if (activations[zone] != ZoneActivation::NEVER) {
			skip = false;
			break;
		}
	}
	if (skip) {
		return Measurement::NONE;
	} else {
		measured_zone = zone;
		select_channel(zone);
		return POLARITY;
	}
}



