//
// Created by independent-variable on 3/25/2024.
//
#include "lcd.h"
#include "lcd_periph.h"

#define FUNCTION_SET	0x20U
#define DB_4BITS	(0 << 4)
#define DB_8BITS	(1U << 4)
#define TWO_LINES	(1U << 3)

#define ENTRY_MODE_SET	0x04U
#define INC_ADDR		(1U << 1)
#define NO_DISP_SHIFT	(0 << 0)

#define SET_CGRAM_ADDR	0x40U

// typical time(in us) for execution - 37us in spec
#define TYP_EXE_TIME	50U
// execution time(in us) for 'return home' and 'clear display' - 1.52ms in spec
#define LONG_EXE_TIME	2000U

static uint8_t cursor_address;

/** 2 lines, cursor at 1st pos. on the 1st row, display is clear and turned off,
 * back light is off */
void lcd::init() {
	TIMER.wait_us(11000); // wait 11ms for power-on(10ms in spec.)
	init_pins();
	switch_to_write(); // read is not supported by hardware

	// set DB to 4bits
	rs_command();
	// first set to 8bits - to sync. communication
	const uint8_t set_8bits = (FUNCTION_SET | DB_8BITS) >> 4;
	// 4 writes because: 3 - in case prev. write(in 4bit config) was interrupted
	// + 1 - in case prev. write was 'data write'
	write_nibble(set_8bits);
	TIMER.wait_us(TYP_EXE_TIME);

	write_nibble(set_8bits);
	TIMER.wait_us(TYP_EXE_TIME);

	write_nibble(set_8bits);
	TIMER.wait_us(TYP_EXE_TIME);

	write_nibble(set_8bits);
	TIMER.wait_us(TYP_EXE_TIME);
	// now set to 4bits
	const uint8_t set_4bits = (FUNCTION_SET | DB_4BITS) >> 4;
	write_nibble(set_4bits);
	TIMER.wait_us(TYP_EXE_TIME);

	// enable 2 lines
	write(FUNCTION_SET | DB_4BITS | TWO_LINES);
	TIMER.wait_us(TYP_EXE_TIME);

	// auto increment addresses, do not shift display
	write(ENTRY_MODE_SET | INC_ADDR | NO_DISP_SHIFT);
	TIMER.wait_us(TYP_EXE_TIME);

	// other configs
	return_home();
	clear_display();
	display_on_off(Screen::OFF, Cursor::OFF, Blinking::OFF);
	cursor_address = 0x00;
}

/** Set address in mem. area for custom char.
 * @param addr 6-bit wide */
static void set_cgram_addr(uint8_t addr) {
	using namespace lcd;
	rs_command();
	write(0x40 | addr);
	TIMER.wait_us(TYP_EXE_TIME);
}

/** Set address in mem. area which is shown on display
 * @param addr 7-bit wide */
static void set_ddram_addr(uint8_t addr) {
	using namespace lcd;
	rs_command();
	write(0x80 | addr);
	TIMER.wait_us(TYP_EXE_TIME);
	cursor_address = addr;
}

void lcd::display_on_off(Screen screen, Cursor cursor, Blinking blinking) {
	rs_command();
	write(0x08 | (uint8_t)screen | (uint8_t)cursor | (uint8_t)blinking);
	TIMER.wait_us(TYP_EXE_TIME);
}

void lcd::return_home() {
	rs_command();
	write(0x02);
	TIMER.wait_us(LONG_EXE_TIME);
	cursor_address = 0;
}

void lcd::clear_display() {
	rs_command();
	write(0x01);
	TIMER.wait_us(LONG_EXE_TIME);
}

/** @param code 3 LSB bits
 * @param bitmap 1st byte - top, last - bottom */
void lcd::create_char(uint8_t code, const uint8_t * bitmap) {
	uint8_t count = LCD_CHAR_HEIGHT;
	set_cgram_addr((0x07U & code) << 3);
	rs_data();
	do {
		write(*bitmap++);
		TIMER.wait_us(TYP_EXE_TIME);
		count -= 1;
	} while (count);
	set_ddram_addr(cursor_address);
}

/** @param pos 0..15 */
void lcd::set_cursor_on_line1(uint8_t pos) {
	set_ddram_addr(pos & 0x0F);
}

/** @param pos 0..15 */
void lcd::set_cursor_on_line2(uint8_t pos) {
	set_ddram_addr(0x40 | (pos & 0x0F));
}

void lcd::print(const char * text) {
	rs_data();
	while (*text) {
		write(*text++);
		TIMER.wait_us(TYP_EXE_TIME);
		cursor_address = next_ddram_address(cursor_address);
	}
}

void lcd::print(char symb) {
	rs_data();
	write(symb);
	TIMER.wait_us(TYP_EXE_TIME);
	cursor_address = next_ddram_address(cursor_address);
}

void lcd::backlight_on() {
	led_on();
}

void lcd::backlight_off() {
	led_off();
}