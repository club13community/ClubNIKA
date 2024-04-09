//
// Created by independent-variable on 3/21/2024.
//

#pragma once
#include <stdint.h>

namespace keyboard {
	enum class Button : uint8_t {N0 = 0, N1, N2, N3, N4, N5, N6, N7, N8, N9, A, B, C, D, STAR, POUND};

	enum class Event : uint8_t {CLICK = 1, LONG_PRESS = 2};

	struct ButtonEvent {
		Button button : 4;
		Event event : 4;

		ButtonEvent() : ButtonEvent(Button::N0, Event::CLICK) {}

		ButtonEvent(Button button, Event event) : button(button), event(event) {}

		ButtonEvent(const ButtonEvent & other) {
			*this = other;
		}

		ButtonEvent & operator=(const ButtonEvent & other) {
			button = other.button;
			event = other.event;
			return *this;
		}

	};

	void init_periph();
	void start();
	void exti_isr();
}
