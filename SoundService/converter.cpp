//
// Created by independent-variable on 5/28/2024.
//
#include "./converter.h"
#include "./converter_periph.h"
#include "./config.h"

using namespace player;

static volatile DataSupplier get_data;

void player::init_converter() {
	init_timer();
	init_dma();
	init_dac();
}

void player::set_sample_rate(uint32_t sample_rate) {
	uint32_t period = (2U * 1'000'000U + sample_rate) / (2U * sample_rate);
	set_sample_period(period);
}

void player::start_conversion(uint8_t * samples, uint16_t size, DataSupplier put_data) {
	get_data = put_data;
	put_initial_samples(samples[0]); // it's simpler to have these 2 extra samples - ok when play sound
	start_dma(samples, size);
	start_timer();
}

void player::set_samples(uint8_t * samples, uint16_t size) {
	start_dma(samples, size);
}

void player::stop_conversion() {
	stop_timer();
	stop_dma();
}

void player::shift_zero_level(uint8_t * samples, uint16_t size) {
	constexpr uint8_t vdd_margin_lsb = MIN_VDD_TO_VOUT_mv * 256U / MIN_VDD_mv;
	constexpr uint8_t gnd_margin_lsb = MIN_VOUT_TO_GND_mV * 256U / MIN_VDD_mv;
	constexpr uint8_t new_zero_level = (255U - vdd_margin_lsb + gnd_margin_lsb) / 2U;
	constexpr int8_t shift = new_zero_level - 127U;
	while (size-- > 0) {
		*samples += shift;
		samples++;
	}
}

void player::dma_isr() {
	begin_dma_isr();
	// last sample is still not played(and will not if new samples are not provided) - ok for playing sound

	uint8_t * samples;
	uint16_t size = 0;
	BaseType_t task_woken = pdFALSE;
	get_data(samples, size, task_woken);

	if (size > 0) {
		start_dma(samples, size);
	}
	portYIELD_FROM_ISR(task_woken);
}