//
// Created by independent-variable on 3/23/2024.
//

#pragma once
#include <stdint.h>

namespace speaker {
	struct Data {
		uint8_t * samples;
		uint16_t samplesLength;

		inline bool isEnd() const {
			return samplesLength == 0;
		}
	};

	constexpr Data DATA_END = {.samples = nullptr, .samplesLength = 0};

	typedef Data (* DataSupplier)();
	typedef void (* Callback)();

	void initPeripherals();
	void playOnDac(uint16_t samplePeriod_us, DataSupplier getData, Callback onEnd);
	void stopPlay();

	void mute();
	void unmute();

	void selectDac();
	void selectSim900();

	void dmaISR();
	void timerISR();
}