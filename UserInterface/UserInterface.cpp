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
#include "timers.h"
#include "event_groups.h"
#include "semphr.h"
#include "ui_private.h"
#include "controllers.h"
#include "display.h"

void user_interface::init_periph() {
	lcd::init();
	keyboard::init_periph();
}

using user_interface::Controller, user_interface::Display;

static TaskHandle_t service;
static StackType_t service_stack[256];
static StaticTask_t service_ctrl;

static Controller * controller;

static EventGroupHandle_t ui_events;
static StaticEventGroup_t ui_events_ctrl;
#define KEYBOARD_EVENT				( (EventBits_t)(1U << 0) )
#define CONTROLLER_CHANGED_EVENT	( (EventBits_t)(1U << 1) )
#define ACTIVITY_CHANGED_EVENT		( (EventBits_t)(1U << 2) )
#define ALL_EVENTS					(KEYBOARD_EVENT | CONTROLLER_CHANGED_EVENT | ACTIVITY_CHANGED_EVENT)

#define KEYBOARD_QUEUE_LENGTH	3U
static uint8_t keyboard_events_data[KEYBOARD_QUEUE_LENGTH * sizeof (keyboard::ButtonEvent)];
static StaticQueue_t keyboard_events_ctrl;
static QueueHandle_t keyboard_events;

static volatile bool ui_is_active;
static volatile bool put_out_display_on_inactivity;
static SemaphoreHandle_t activity_mutex;
static StaticSemaphore_t activity_mutex_ctrl;
static TimerHandle_t activity_timer;
static StaticTimer_t activity_timer_ctrl;
/** How long user may not interact with UI and display's backlight will stay on */
#define ACTIVITY_TIMEOUT_ms	20000U

static void on_activity_timeout(TimerHandle_t xTimer) {
	if (xSemaphoreTake(activity_mutex, 0) == pdFALSE) {
		return;
	}
	ui_is_active = false;
	xEventGroupSetBits(ui_events, ACTIVITY_CHANGED_EVENT);
	xSemaphoreGive(activity_mutex);
}

static bool get_and_report_activity() {
	while (xSemaphoreTake(activity_mutex, portMAX_DELAY) == pdFALSE);

	bool ui_was_active = ui_is_active;
	xTimerReset(activity_timer, portMAX_DELAY);
	ui_is_active = true;
	if (ui_was_active != ui_is_active) {
		xEventGroupSetBits(ui_events, ACTIVITY_CHANGED_EVENT);
	}

	xSemaphoreGive(activity_mutex);
	return ui_was_active;
}

/** First press of a button will light up backlight again, this pressing of a button will not be handled - just lights up.*/
void user_interface::handle(keyboard::ButtonEvent button_event) {
	xQueueSend(keyboard_events, &button_event, 0);
	xEventGroupSetBits(ui_events, KEYBOARD_EVENT);
}

void Controller::activate_next(Controller * next) {
	portENTER_CRITICAL();
	next->previous = controller;
	controller = next;
	xEventGroupSetBits(ui_events, CONTROLLER_CHANGED_EVENT);
	portEXIT_CRITICAL();
}

void Controller::activate_previous() {
	portENTER_CRITICAL();
	controller = controller->previous;
	xEventGroupSetBits(ui_events, CONTROLLER_CHANGED_EVENT);
	portEXIT_CRITICAL();
}

bool Controller::get_and_report_activity() {
	return ::get_and_report_activity();
}

/** Put out backlight when no button is pressed(or no other activity reported) for a long time.
 * Invoke this method before controlling backlight manually. */
Display & Display::put_out_on_inactivity() {
	put_out_display_on_inactivity = true;
	return *this;
}

/** Control backlight manually - no automatic backlight off.
 * Invoke this method before controlling backlight manually.*/
Display & Display::put_out_manually() {
	put_out_display_on_inactivity = false;
	return *this;
}

static void service_ui(void * args) {
	using namespace user_interface;
	display_on_off(lcd::Screen::ON, lcd::Cursor::OFF, lcd::Blinking::OFF);
	while (true) {
		const BaseType_t dont_clear_bits = pdFALSE;
		const BaseType_t wait_for_any_bit = pdFALSE;
		EventBits_t events = xEventGroupWaitBits(ui_events, ALL_EVENTS, dont_clear_bits, wait_for_any_bit, portMAX_DELAY);
		if (events & CONTROLLER_CHANGED_EVENT) {
			ui_is_active = true;
			xTimerReset(activity_timer, portMAX_DELAY);
			xQueueReset(keyboard_events);
			xEventGroupClearBits(ui_events, ALL_EVENTS);
			controller->activate();
		} else if (events & ACTIVITY_CHANGED_EVENT) {
			bool is_active = ui_is_active;
			if (put_out_display_on_inactivity) {
				if (is_active) {
					lcd::backlight_on();
				} else {
					lcd::backlight_off();
				}
			}
			xEventGroupClearBits(ui_events, ACTIVITY_CHANGED_EVENT);
			if (!is_active) {
				controller->on_ui_inactive();
			}
		} else if (events & KEYBOARD_EVENT) {
			keyboard::ButtonEvent buttonEvent;
			if (xQueueReceive(keyboard_events, &buttonEvent, 0) == pdFALSE) {
				xEventGroupClearBits(ui_events, KEYBOARD_EVENT);
			} else if (get_and_report_activity()) {
				// handle button only if UI was active before
				controller->handle(buttonEvent.button, buttonEvent.event);
			}
		}
	}
}

void user_interface::start() {
	keyboard_events = xQueueCreateStatic(KEYBOARD_QUEUE_LENGTH, sizeof (keyboard::ButtonEvent),
										 keyboard_events_data, &keyboard_events_ctrl);
	activity_mutex = xSemaphoreCreateMutexStatic(&activity_mutex_ctrl);
	activity_timer = xTimerCreateStatic("ui activity", pdMS_TO_TICKS(ACTIVITY_TIMEOUT_ms), pdFALSE, nullptr,
										on_activity_timeout, &activity_timer_ctrl);
	service = xTaskCreateStatic(service_ui, "ui service", 256, nullptr, TASK_NORMAL_PRIORITY,
								service_stack, &service_ctrl);
	ui_events = xEventGroupCreateStatic(&ui_events_ctrl);
	controller = desktop;
	xEventGroupSetBits(ui_events, CONTROLLER_CHANGED_EVENT);

	keyboard::start();
}
/*
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
 */