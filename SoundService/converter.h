//
// Created by independent-variable on 5/28/2024.
//

#pragma once
#include <stdint.h>
#include "FreeRTOS.h"

namespace player {
	constexpr uint32_t min_sample_rate = 16, max_sample_rate = 100'000U;
	/** Supplies new samples inside ISR. Set size = 0 and explicitly stop playing if there are no more samples. */
	typedef void (* DataSupplier)(uint8_t * & samples, uint16_t & size, BaseType_t & task_woken);

	void init_converter();

	/** @param sample_rate in range [16, 100k]. Sampling period error will be < 5%.
 	* At 44.1KHz - 2.2%, at 20KHz - 1%. */
	void set_sample_rate(uint32_t sample_rate);
	void start_conversion(uint8_t * samples, uint16_t size, DataSupplier put_data);
	void stop_conversion();
	/** Use this method to supply new samples if DataSupplier failed to obtain new samples in time.
	 * Last sample from prev. chunk is played during all this time. */
	void set_samples(uint8_t * samples, uint16_t size);

	/** Converter output can not be rail to rail(HW limitation). Code which loads samples should use this method
	 * to shift 0-level of samples before passing to be played. Assumed, that current 0-level is 127. */
	void shift_zero_level(uint8_t * samples, uint16_t size);

	void dma_isr();
}
