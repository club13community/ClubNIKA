//
// Created by independent-variable on 3/15/2024.
//

#pragma once
#include <stdint.h>

namespace timing {
	typedef void (* Callback)();

	/** Allows transitions between modes. Callback from prev. configuration still may be invoked once. */
	class CoarseTimer {
	public:
		virtual void invoke_in_ms(uint16_t delayMs, Callback callback) = 0;
		virtual void invoke_in_ticks(uint16_t delayMsDiv4, Callback callback) = 0;
		virtual void every_ms_invoke(uint16_t periodMs, Callback callback) = 0;
		virtual void every_ticks_invoke(uint16_t periodMsDiv4, Callback callback) = 0;
		virtual void wait_ms(uint16_t delayMs) = 0;
		virtual void wait_ticks(uint16_t delayMsDiv4) = 0;
		virtual void stop() = 0;
	};

	/** Allows transitions between modes. Callback from prev. configuration still may be invoked once. */
	class FineTimer {
	public:
		virtual void invoke_in_us(uint16_t delayUs, Callback callback) = 0;
		virtual void every_us_invoke(uint16_t periodUs, Callback callback) = 0;
		virtual void wait_us(uint16_t delayUs) = 0;
		virtual void stop() = 0;

	};

	void config_coarse_timer();
	void config_fine_timer();
	void handle_coarse_timer_interrupt();
	void handle_fine_timer_interrupt();

	extern CoarseTimer & coarse_timer1;
	extern CoarseTimer & coarse_timer2;
	extern CoarseTimer & coarse_timer3;
	extern CoarseTimer & coarse_timer4;

	extern FineTimer & fine_timer1;
	extern FineTimer & fine_timer2;
	extern FineTimer & fine_timer3;
	extern FineTimer & fine_timer4;
}