//
// Created by independent-variable on 5/28/2024.
//

#pragma once
#include <stdint.h>
#include "FreeRTOS.h"

namespace speaker {
	/** Supplies new samples inside ISR. Set size = 0 and explicitly stop playing if there are no more samples. */
	typedef void (* DataSupplier)(uint8_t * & samples, uint16_t & size, BaseType_t & task_woken);

	void init_converter();

	void set_sample_rate(uint32_t sample_rate);
	void start_playing(uint8_t * samples, uint16_t size, DataSupplier put_data);
	void stop_playing();
	/** Use this method to supply new samples if DataSupplier failed to obtain new samples in time.
	 * Last sample from prev. chunk is played during all this time. */
	void keep_playing(uint8_t * samples, uint16_t size);

	void dma_isr();
}
