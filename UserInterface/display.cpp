//
// Created by independent-variable on 4/5/2024.
//
#include "display.h"
#include "lcd.h"

using namespace user_interface;

Display & Display::light_up() {
	lcd::backlight_on();
	return *this;
}

Display & Display::put_out() {
	lcd::backlight_off();
	return *this;
}

Display & Display::clear() {
	lcd::clear_display();
	return *this;
}

Display & Display::cursor(uint8_t line, uint8_t pos) {
	lcd::set_cursor(line, pos);
	return *this;
}

/** Moves cursor in row
 * @param offset -15..15 */
Display & Display::move(int8_t offset) {
	if (offset < -15) {
		offset = -15;
	} else if (offset > 15) {
		offset = 15;
	}
	int8_t position = lcd::get_position() + offset;
	if (position > 15) {
		position = 15;
	} else if (position < 0) {
		position = 0;
	}
	lcd::set_cursor(position);
	return *this;
}

Display & Display::push_cursor() {
	cursor_stack = lcd::get_cursor_addr();
	return *this;
}

Display & Display::pop_cursor() {
	lcd::set_cursor_addr(cursor_stack);
	return *this;
}

Display & Display::define(char code, const uint8_t * bitmap) {
	lcd::create_char(code, bitmap);
	return *this;
}

Display & Display::print(char symb) {
	if (symb == endl) {
		// move to the beginning of other line
		lcd::set_cursor(lcd::get_line() ^ 0x01U, 0);
	} else {
		lcd::print(symb);
	}
	return *this;
}

Display & Display::print(const char * text) {
	lcd::print(text);
	return *this;
}

Display & Display::print(int num) {
	bool sign = false;
	if (num < 0) {
		num = -num;
		sign = true;
	}
	char text[10];
	uint8_t len = 0;
	do {
		text[len++] = 0x30 | (num % 10);
		num /= 10;
	} while (num);
	if (sign) {
		text[len++] = '-';
	}
	for (uint8_t i = 0, j = len - 1; i < (len >> 1); i++, j--) {
		char c = text[i];
		text[i] = text[j];
		text[j] = c;
	}
	text[len] = 0;
	lcd::print(text);
	return *this;
}

Display & Display::operator[](uint8_t pos) {
	lcd::set_cursor(pos);
	return *this;
}

namespace user_interface {
	Display disp;
}