//
// Created by independent-variable on 4/5/2024.
//

#pragma once
#include <stdint.h>

/** Helper class, which provides more convenient way for controllers to control LCD display */
namespace user_interface {
	class Display {
	private:
		uint8_t cursor_stack = 0;
	public:
		Display & light_up();
		Display & put_out();
		/** Clear all display. */
		Display & clear();
		/** @param positions number of positions to be cleared in a current row starting from current cursor. */
		Display & clear(uint8_t positions);
		Display & set_cursor(uint8_t line, uint8_t pos);
		Display & move_cursor(int8_t offset);
		Display & push_cursor();
		Display & pop_cursor();
		Display & define(char code, const uint8_t * bitmap);
		Display & print(char symb);
		Display & print(const char * text);
		Display & print(int num);

		Display & operator<<(char symb) {
			return print(symb);
		}

		Display & operator<<(const char * text) {
			return print(text);
		}

		Display & operator<<(int num) {
			return print(num);
		}

		Display & operator[](uint8_t pos);
	};

	extern Display disp;

	const char endl = '\n';
}
