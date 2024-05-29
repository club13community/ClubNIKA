//
// Created by independent-variable on 5/28/2024.
//
#include "./converter.h"
#include "./converter_periph.h"

using namespace speaker;

static volatile DataSupplier get_data;

void speaker::init_converter() {
	init_timer();
	init_dma();
	init_dac();
}

/** @param sample_rate in range [16, 100k]. Sampling period error will be < 5%.
 * At 44.1KHz - 2.2%, at 20KHz - 1%. */
void speaker::set_sample_rate(uint32_t sample_rate) {
	uint32_t period = (2U * 1'000'000U + sample_rate) / (2U * sample_rate);
	set_sample_period(period);
}

void speaker::start_playing(uint8_t * samples, uint16_t size, DataSupplier put_data) {
	get_data = put_data;
	put_initial_samples(samples[0]); // it's simpler to have these 2 extra samples - ok when play sound
	start_dma(samples, size);
	start_timer();
}

void speaker::keep_playing(uint8_t * samples, uint16_t size) {
	start_dma(samples, size);
}

void speaker::stop_playing() {
	stop_timer();
	stop_dma();
}

void speaker::dma_isr() {
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