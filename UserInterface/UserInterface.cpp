/*
 * UserInterface.c
 *
 *  Created on: 3 квіт. 2020 р.
 *      Author: MaxCm
 */

#include "UserInterface.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lcd.h"
#include "keyboard.h"
#include "periph_allocation.h"
#include "queue.h"

void user_interface::init_periph() {
	lcd::init();
	keyboard::init_periph();
}

uint8_t bat[] = {
		0b01110,
		0b11111,
		0b10001,
		0b11111,
		0b10001,
		0b11111,
		0b10001,
		0b11111
};

uint8_t sign[] = {
		0b11111,
		0b11111,
		0b00000,
		0b01110,
		0b01110,
		0b00000,
		0b00100,
		0b00100
};

static TaskHandle_t service;
static StackType_t service_stack[256];
static StaticTask_t service_ctrl;

static void service_ui(void * args) {
	using namespace lcd;
	display_on_off(Screen::ON, Cursor::OFF, Blinking::OFF);
	set_cursor_on_line1(1);
	print("I am");

	set_cursor_on_line1(7);

	create_char(0, bat);
	print('\0');

	set_cursor_on_line2(2);

	create_char(7, sign);
	print('\7');

	set_cursor_on_line2(15 - 7);
	print("working");
	backlight_on();

	while(true) {
		using keyboard::Button;
		keyboard::ButtonEvent event;
		if (xQueueReceive(keyboard::get_button_events(), &event, 0) == pdFALSE) {
			taskYIELD();
		} else {
			set_cursor_on_line1(15);
			char symb;
			if (event.button >= Button::N0 && event.button <= Button::N9) {
				symb = (uint8_t)event.button | 0x30U;
			} else if (event.button >= Button::A && event.button <= Button::D) {
				symb = (uint8_t)event.button - (uint8_t)Button::A + 0x41U;
			} else if (event.button == Button::STAR) {
				symb = '*';
			} else {
				symb = '#';
			}
			print(symb);
		}
	}
}

void user_interface::start() {
	keyboard::start();
	service = xTaskCreateStatic(service_ui, "ui service", 256, nullptr, TASK_NORMAL_PRIORITY, service_stack, &service_ctrl);
}
