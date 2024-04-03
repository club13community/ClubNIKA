//
// Created by independent-variable on 3/30/2024.
//
#include "voltage_meter.h"
#include "periph.h"
#include "wired_zones_meas.h"
#include "supply_system_meas.h"

// how many samples for averaging: 2^val
#define AVERAGING_pow2	2

#define AVERAGING_COUNT	(1U << AVERAGING_pow2)

void vmeter::init_periph() {
	init_pins();
	init_timer();
	init_adc();
	power_up_adc();
	calibrate_adc();
	enable_adc_trigger();
	enable_adc_irq();
}

volatile uint8_t zone_samples_count;
volatile uint32_t zone_samples_acc;

volatile uint8_t battery_samples_count;
volatile uint32_t battery_samples_acc;

static void request_zone_measurement() {
	using wired_zones::Measurement, wired_zones::select_measurement;
	using vmeter::measure_zone_sourcing, vmeter::measure_zone_sinking, vmeter::disable_zone_measurement;

	Measurement meas = select_measurement();
	if (meas == Measurement::SINKING) {
		measure_zone_sinking();
		zone_samples_count = AVERAGING_COUNT;
		zone_samples_acc = 0;
	} else if (meas == Measurement::SOURCING) {
		measure_zone_sourcing();
		zone_samples_count = AVERAGING_COUNT;
		zone_samples_acc = 0;
	} else {
		disable_zone_measurement();
	}
}

void vmeter::start() {
	request_zone_measurement();
	battery_samples_acc = 0;
	battery_samples_count = AVERAGING_COUNT;
	start_conversions();
}

#define to_mV(val) ((uint32_t)val * 3000U >> 12)

void vmeter::isr() {

	bool measure_zones = ADC->SR & ADC_SR_JEOC;

	if (measure_zones) {
		zone_samples_acc += to_mV(get_zone_measurement());
		zone_samples_count--;
		if (!zone_samples_count) {
			wired_zones::process_measurement(zone_samples_acc >> AVERAGING_pow2);
			request_zone_measurement();
		}
	} else {
		request_zone_measurement();
	}

	battery_samples_acc += to_mV(get_battery_measurement());
	battery_samples_count--;
	if (!battery_samples_count) {
		supply_system::process_battery_measurement(battery_samples_acc >> AVERAGING_pow2);
		battery_samples_acc = 0;
		battery_samples_count = AVERAGING_COUNT;
	}

	clear_flags();
}