//
// Created by independent-variable on 3/15/2024.
//

#pragma once

namespace timing {
	typedef void (* CallbackPointer)();

	class Timer {
	public:
		virtual void invoke_in_ms(uint16_t delayMs, CallbackPointer callback) = 0;
		virtual void invoke_in_ticks(uint16_t delayMsDiv4, CallbackPointer callback) = 0;
		virtual void every_ms_invoke(uint16_t periodMs, CallbackPointer callback) = 0;
		virtual void every_ticks_invoke(uint16_t periodMsDiv4, CallbackPointer callback) = 0;
		virtual void wait_ms(uint16_t delayMs) = 0;
		virtual void wait_ticks(uint16_t delayMsDiv4) = 0;
		virtual void stop() = 0;
	};

	void configPeripherals();
	void handleIrq();

	extern Timer & timer1;
	extern Timer & timer2;
	extern Timer & timer3;
	extern Timer & timer4;
}