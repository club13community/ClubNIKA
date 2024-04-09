//
// Created by independent-variable on 3/25/2024.
//

#pragma once
#include <stdint.h>

#define LCD_CHAR_HEIGHT	8U

namespace lcd {
	enum class Screen : uint8_t {OFF = 0 << 2, ON = 1U << 2};
	enum class Cursor : uint8_t {OFF = 0 << 1, ON = 1U << 1};
	enum class Blinking: uint8_t {OFF = 0 << 0, ON = 1U << 0};

	void init();
	void display_on_off(Screen screen, Cursor cursor, Blinking blinking);
	void create_char(uint8_t code, const uint8_t * bitmap);
	void set_cursor(uint8_t pos);
	void set_cursor(uint8_t line, uint8_t pos);
	void set_cursor_on_line1(uint8_t pos);
	void set_cursor_on_line2(uint8_t pos);
	uint8_t get_line();
	uint8_t get_position();
	uint8_t get_cursor_addr();
	void set_cursor_addr(uint8_t addr);
	void return_home();
	void clear_display();
	void print(const char * text);
	void print(char symb);
	void backlight_on();
	void backlight_off();
}