/*
 * UserInterface.c
 *
 *  Created on: 3 квіт. 2020 р.
 *      Author: MaxCm
 */

#include "UserInterface.h"
#include "FreeRTOS.h"
#include "task.h"
#include "./lcd.h"
#include "keyboard.h"
#include "periph_allocation.h"
#include "queue.h"
#include "timers.h"
#include "event_groups.h"
#include "./controllers.h"
#include "./display.h"

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
#define KEYBOARD_EVENT			( (EventBits_t)(1U << 0) )
#define INVOKE_CONTROLLER_EVENT	( (EventBits_t)(1U << 1) )
#define RESUME_CONTROLLER_EVENT	( (EventBits_t)(1U << 2) )
#define ACTIVITY_TIMEOUT_EVENT	( (EventBits_t)(1U << 3) )
#define DELAY_TIMEOUT_EVENT		( (EventBits_t)(1U << 4) )

#define CONTROLLER_CHANGED_EVENTS	(INVOKE_CONTROLLER_EVENT | RESUME_CONTROLLER_EVENT)
#define TIMER_EVENTS				(ACTIVITY_TIMEOUT_EVENT | DELAY_TIMEOUT_EVENT)
#define ALL_EVENTS					(KEYBOARD_EVENT | CONTROLLER_CHANGED_EVENTS | TIMER_EVENTS)

#define KEYBOARD_QUEUE_LENGTH	3U
static uint8_t keyboard_events_data[KEYBOARD_QUEUE_LENGTH * sizeof (keyboard::ButtonEvent)];
static StaticQueue_t keyboard_events_ctrl;
static QueueHandle_t keyboard_events;

static TimerHandle_t activity_timer;
static StaticTimer_t activity_timer_ctrl;
static TimerHandle_t delay_timer;
static StaticTimer_t delay_timer_ctrl;
/** How long user may not interact with UI and display's backlight will stay on */
#define ACTIVITY_TIMEOUT_ms	20000U

static void on_activity_timeout(TimerHandle_t xTimer) {
	xEventGroupSetBits(ui_events, ACTIVITY_TIMEOUT_EVENT);
}

static void on_delay_timeout(TimerHandle_t xTimer) {
	xEventGroupSetBits(ui_events, DELAY_TIMEOUT_EVENT);
}

void handle_keyboard(keyboard::ButtonEvent button_event) {
	xQueueSend(keyboard_events, &button_event, 0);
	xEventGroupSetBits(ui_events, KEYBOARD_EVENT);
}

void Controller::invoke() {
	run_activity_timer = true;
	ui_active = true;
	delay_ms = 0;
	portENTER_CRITICAL(); // timer events will be served with single context switching
	xTimerReset(activity_timer, portMAX_DELAY); // also starts timer
	xTimerStop(delay_timer, portMAX_DELAY);
	xEventGroupClearBits(ui_events, TIMER_EVENTS);
	portEXIT_CRITICAL();
	disp.light_up();
	activate(true);
}

void Controller::resume() {
	ui_active = true;
	portENTER_CRITICAL(); // timer events will be served with single context switching
	if (run_activity_timer) {
		xTimerReset(activity_timer, portMAX_DELAY);
	} else {
		xTimerStop(activity_timer, portMAX_DELAY);
	}
	if (delay_ms > 0) {
		xTimerChangePeriod(delay_timer, pdMS_TO_TICKS(delay_ms), portMAX_DELAY); // this also starts timer
	} else {
		xTimerStop(delay_timer, portMAX_DELAY);
	}
	xEventGroupClearBits(ui_events, TIMER_EVENTS);
	portEXIT_CRITICAL();
	disp.light_up();
	activate(false);
}

void Controller::ui_activated() {
	// may be invoked by keyboard event or delay timer
	if (run_activity_timer) {
		portENTER_CRITICAL();
		xTimerReset(activity_timer, portMAX_DELAY);
		xEventGroupClearBits(ui_events, ACTIVITY_TIMEOUT_EVENT);
		portEXIT_CRITICAL();
	}
	if (!ui_active) {
		// it is false - so activity timer is running
		ui_active = true;
		disp.light_up();
	}
}

void Controller::ui_suspended() {
	// may be invoked only by activity timer
	ui_active = false;
	disp.put_out();
}

void Controller::invoke(Controller * next) {
	next->previous = controller;
	controller = next;
	xEventGroupSetBits(ui_events, INVOKE_CONTROLLER_EVENT);
}

void Controller::yield() {
	controller = controller->previous;
	xEventGroupSetBits(ui_events, RESUME_CONTROLLER_EVENT);
}

void Controller::handle_ui_inactivity(bool do_handle) {
	if (do_handle && !run_activity_timer) {
		run_activity_timer = true;
		xTimerReset(activity_timer, portMAX_DELAY);
		// timer was dormant - no need to clear event bit and ui_active = true;
	} else if (!do_handle && run_activity_timer) {
		ui_active = true; // this unblocks keyboard, display is controlled manually
		run_activity_timer = false;
		xTimerStop(activity_timer, portMAX_DELAY);
		xEventGroupClearBits(ui_events, ACTIVITY_TIMEOUT_EVENT);
	} // else - already handling or not handling
}

void Controller::start_delay(uint16_t delay_ms) {
	this->delay_ms = delay_ms;
	portENTER_CRITICAL();
	xTimerChangePeriod(delay_timer, pdMS_TO_TICKS(delay_ms), portMAX_DELAY); // this also starts timer
	xEventGroupClearBits(ui_events, DELAY_TIMEOUT_EVENT);
	portEXIT_CRITICAL();
}

void Controller::cancel_delay() {
	delay_ms = 0;
	xTimerStop(delay_timer, portMAX_DELAY);
	xEventGroupClearBits(ui_events, DELAY_TIMEOUT_EVENT);
}

static void service_ui_events(void * args) {
	display_on_off(lcd::Screen::ON, lcd::Cursor::OFF, lcd::Blinking::OFF);
	controller->invoke();
	while (true) {
		const BaseType_t dont_clear_bits = pdFALSE;
		const BaseType_t wait_for_any_bit = pdFALSE;
		EventBits_t bits = xEventGroupWaitBits(ui_events, ALL_EVENTS, dont_clear_bits, wait_for_any_bit, portMAX_DELAY);
		if (bits & CONTROLLER_CHANGED_EVENTS) {
			xQueueReset(keyboard_events);
			xEventGroupClearBits(ui_events, CONTROLLER_CHANGED_EVENTS | KEYBOARD_EVENT);
			if (bits & INVOKE_CONTROLLER_EVENT) {
				controller->invoke();
			} else {
				controller->resume();
			}
		} else if (bits & ACTIVITY_TIMEOUT_EVENT) {
			xEventGroupClearBits(ui_events, ACTIVITY_TIMEOUT_EVENT);
			controller->on_activity_timer();
		} else if (bits & DELAY_TIMEOUT_EVENT) {
			xEventGroupClearBits(ui_events, DELAY_TIMEOUT_EVENT);
			controller->on_delay_timer();
		} else if (bits & KEYBOARD_EVENT) {
			keyboard::ButtonEvent buttonEvent;
			if (xQueueReceive(keyboard_events, &buttonEvent, 0) == pdFALSE) {
				xEventGroupClearBits(ui_events, KEYBOARD_EVENT);
			} else {
				controller->on_keyboard(buttonEvent.button, buttonEvent.event);
			}
		}
	}
}

void user_interface::start() {
	keyboard_events = xQueueCreateStatic(KEYBOARD_QUEUE_LENGTH, sizeof (keyboard::ButtonEvent),
										 keyboard_events_data, &keyboard_events_ctrl);
	activity_timer = xTimerCreateStatic("ui activity", pdMS_TO_TICKS(ACTIVITY_TIMEOUT_ms), pdFALSE, nullptr,
										on_activity_timeout, &activity_timer_ctrl);
	delay_timer = xTimerCreateStatic("ui delay", pdMS_TO_TICKS(1), pdFALSE, nullptr,
									 	on_delay_timeout, &delay_timer_ctrl);
	service = xTaskCreateStatic(service_ui_events, "ui service", 256, nullptr, TASK_NORMAL_PRIORITY,
								service_stack, &service_ctrl);
	ui_events = xEventGroupCreateStatic(&ui_events_ctrl);
	controller = desktop;

	keyboard::start();
}