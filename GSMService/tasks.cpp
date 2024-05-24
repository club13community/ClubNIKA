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
static volatile TimerHandle_t module_state_timer;

static inline void schedule_module_state_update() {
	xTimerReset(module_state_timer, portMAX_DELAY);
}

static void module_status_timeout(TimerHandle_t timer);

void gsm::init_task_execution() {
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
	xTimerStop(module_state_timer, portMAX_DELAY);
	pending_tasks = 0;
}

static void reboot();
static void check_module_state();

void gsm::execute_scheduled() {
	uint16_t pending_now = pending_tasks;
	if (pending_now & to_int(Task::REBOOT)) {
		reboot();
	} else if (pending_now & to_int(Task::CHECK_MODULE_STATE)) {
		check_module_state();
	} else {
		xSemaphoreGive(ctrl_mutex);
	}
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
		reboot_tasks();
		reboot_event_handling();
		reboot_state();
		turn_on(turned_on);
	};

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

static void module_status_timeout(TimerHandle_t timer) {
	execute_or_schedule(Task::CHECK_MODULE_STATE);
}