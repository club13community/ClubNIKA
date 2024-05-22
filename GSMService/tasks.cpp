//
// Created by independent-variable on 5/21/2024.
//
#include "sim900.h"
#include "concurrent_utils.h"
#include "./state.h"
#include "./tasks.h"
#include "./event_handling.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "timers.h"
#include "./config.h"
#include <string.h>
#include "logging.h"

static volatile uint8_t pending_tasks;
static volatile TimerHandle_t call_status_timer;
static volatile TimerHandle_t module_state_timer;

static inline void schedule_call_state_update() {
	xTimerReset(call_status_timer, portMAX_DELAY);
}

static inline void cancel_call_state_update() {
	using gsm::Task;
	xTimerStop(call_status_timer, portMAX_DELAY);
	atomic_clear((uint8_t *)&pending_tasks, ~(uint8_t)Task::CHECK_CALL_STATE);
}

static inline void schedule_module_state_update() {
	xTimerReset(module_state_timer, portMAX_DELAY);
}

static void call_state_timeout(TimerHandle_t timer);
static void module_status_timeout(TimerHandle_t timer);

void gsm::init_task_execution() {
	static StaticTimer_t call_status_timer_ctrl;
	call_status_timer = xTimerCreateStatic("gsm call", pdMS_TO_TICKS(CALL_STATUS_UPDATE_PERIOD_ms),
										   pdFALSE, (void *) 0, call_state_timeout, &call_status_timer_ctrl);

	static StaticTimer_t module_status_timer_ctrl;
	module_state_timer = xTimerCreateStatic("gsm status", pdMS_TO_TICKS(MODULE_STATUS_UPDATE_PERIOD_ms),
											pdFALSE, (void *)0, module_status_timeout, &module_status_timer_ctrl);
}

static inline uint8_t to_int(gsm::Task task) {
	return (uint8_t)task;
}

void gsm::execute_or_schedule(Task task) {
	atomic_set(&pending_tasks, to_int(task));
	if (xSemaphoreTake(ctrl_mutex, 0) == pdTRUE) {
		execute_scheduled();
	}
}

static inline void reboot_tasks() {
	xTimerStop(call_status_timer, portMAX_DELAY);
	xTimerStop(module_state_timer, portMAX_DELAY);
	pending_tasks = 0;
}

static void reboot();
static void check_module_state();
/** First task in incoming call handling. Gets phone number and notifies "event handler" about the call. */
static void get_incoming_number();
static void check_call_state();
/** Should be invoked when interlocutor ends a call(see other for "no answer" and "line busy" scenarios). */
static void notify_call_ended();


void gsm::execute_scheduled() {
	uint16_t pending_now = pending_tasks;
	if (pending_now & to_int(Task::REBOOT)) {
		reboot();
	} else if (pending_now & to_int(Task::CHECK_MODULE_STATE)) {
		check_module_state();
	} else if (pending_now & to_int(Task::GET_INCOMING_PHONE)) {
		get_incoming_number();
	} else if (pending_now & to_int(Task::CHECK_CALL_STATE)) {
		check_call_state();
	} else if (pending_now & to_int(Task::NOTIFY_CALL_ENDED)) {
		notify_call_ended();
	} else {
		xSemaphoreGive(ctrl_mutex);
	}
}

void gsm::poll_call_status() {
	schedule_call_state_update();
}

static inline void executed(gsm::Task task) {
	atomic_clear(&pending_tasks, ~to_int(task));
	gsm::execute_scheduled();
}

using namespace gsm;

static inline bool should_reboot(sim900::Result res) {
	return res == sim900::Result::NO_RESPONSE || res == sim900::Result::CORRUPTED_RESPONSE;
}

void gsm::turn_module_on() {
	using namespace sim900;

	static constexpr auto card_status_received = [](CardStatus status, Result res) {
		if (should_reboot(res)) {
			reboot();
		} else {
			if (res == Result::OK) {
				card_status = status;
			}
			schedule_module_state_update();
			xSemaphoreGive(ctrl_mutex);
		}
	};

	static constexpr auto turned_on = [](bool ok) {
		if (ok) {
			get_card_status(card_status_received);
		} else {
			rec::log("Failed to turn SIM900 on");
			reboot();
		}
	};

	sim900::turn_on(turned_on);
}

static void reboot() {
	using namespace sim900;

	static constexpr auto card_status_received = [](CardStatus status, Result res) {
		if (should_reboot(res)) {
			reboot();
		} else {
			if (res == Result::OK) {
				card_status = status;
			}
			schedule_module_state_update();
			executed(Task::REBOOT);
		}
	};

	static constexpr auto turned_on = [](bool ok) {
		if (ok) {
			get_card_status(card_status_received);
		} else {
			rec::log("Failed to reboot SIM900");
			reboot();
		}
	};

	static constexpr auto turned_off = []() {
		turn_on(turned_on);
	};

	reboot_tasks();
	reboot_event_handling();
	reboot_state();
	turn_off(turned_off);
}

static void check_module_state() {
	using namespace sim900;

	static constexpr auto signal_checked = [](uint8_t signal_pct, Result res) {
		if (res == Result::OK) {
			signal_strength = signal_pct;
			schedule_module_state_update();
			executed(Task::CHECK_MODULE_STATE);
		} else if (should_reboot(res)) {
			handle(Event::ERROR);
			executed(Task::CHECK_MODULE_STATE);
			// do not schedule next update
		} else {
			schedule_module_state_update();
			executed(Task::CHECK_MODULE_STATE);
		}
	};

	static constexpr auto reg_checked = [](Registration new_reg, Result res) {
		if (res == Result::OK) {
			Registration prev_reg = registration;
			registration = new_reg;
			if (new_reg == Registration::DONE) {
				get_signal_strength(signal_checked);
			} else {
				bool was_ok = prev_reg != Registration::FAILED;
				bool ok_now = new_reg != Registration::FAILED;
				if (was_ok && !ok_now) {
					handle(Event::NETWORK_ERROR);
				}
				schedule_module_state_update();
				executed(Task::CHECK_MODULE_STATE);
			}
		} else if (should_reboot(res)) {
			handle(Event::ERROR);
			executed(Task::CHECK_MODULE_STATE);
			// don't schedule next update
		} else {
			schedule_module_state_update();
			executed(Task::CHECK_MODULE_STATE);
		}
	};

	static constexpr auto sim_checked = [](CardStatus new_status, Result res) {
		if (res == Result::OK) {
			CardStatus prev_status = card_status;
			card_status = new_status;
			if (new_status == CardStatus::READY) {
				get_registration(reg_checked);
			} else {
				if (prev_status == CardStatus::READY) {
					// card failure
					handle(Event::CARD_ERROR);
				}
				schedule_module_state_update();
				executed(Task::CHECK_MODULE_STATE);
			}
		} else if (should_reboot(res)) {
			handle(Event::ERROR);
			executed(Task::CHECK_MODULE_STATE);
			// do not schedule next update
		} else {
			schedule_module_state_update();
			executed(Task::CHECK_MODULE_STATE);
		}
	};

	get_card_status(sim_checked);
}

static void get_incoming_number() {
	using namespace sim900;

	static constexpr auto number_retrieved = [](CallState state, char * num, Result res) {
		if (res == Result::OK) {
			if (state != CallState::ENDED) {
				strcpy(phone_number, num);
				schedule_call_state_update();
				call_handling = CallHandling::DIALING;
				handle(Event::INCOMING_CALL);
			} // else - already ended(rang for several milliseconds) - ignore this call
		} else if (should_reboot(res)) {
			handle(Event::ERROR);
		} // else - "ERROR"(should newer be received), still allow retry on next "RING"
		executed(Task::GET_INCOMING_PHONE);
	};

	if (call_handling == CallHandling::FREE) {
		get_call_info(number_retrieved);
	} else {
		// this is repeated "RING"
		executed(Task::GET_INCOMING_PHONE);
	}
}

static void check_call_state() { // todo rename to "check_dialing"
	using namespace sim900;

	static constexpr auto call_checked = [](CallState state, char * num, Result res) {
		if (res == Result::OK) {
			CallHandling call_handling_now = call_handling;
			if (call_handling_now == CallHandling::DIALING) {
				if (state == CallState::DIALED) {
					call_handling = CallHandling::SPEAKING;
					handle(Event::CALL_DIALED);
				} else {
					schedule_module_state_update();
				}
			}
		} else if (should_reboot(res)) {
			handle(Event::ERROR);
		} else {
			schedule_call_state_update(); // will check next time
		}
		executed(Task::CHECK_CALL_STATE);
	};

	get_call_info(call_checked);
}

static void notify_call_ended() {
	CallHandling call_handling_now = call_handling;
	if (call_handling_now == CallHandling::DIALING || call_handling_now == CallHandling::SPEAKING) {
		cancel_call_state_update();
		call_handling = CallHandling::ENDING;
		handle(Event::CALL_ENDED);
	}
	// else - not realistic cases:
	// 1) call ends before get_incoming_number() executed - it will ignore such a short call
	// 2) 1st call ends and other start-end while "event handler" processes 1st call
}

static void call_state_timeout(TimerHandle_t timer) {
	execute_or_schedule(Task::CHECK_CALL_STATE);
}

static void module_status_timeout(TimerHandle_t timer) {
	execute_or_schedule(Task::CHECK_MODULE_STATE);
}