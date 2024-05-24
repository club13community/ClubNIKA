//
// Created by independent-variable on 5/21/2024.
//
#include "concurrent_utils.h"
#include "./state.h"
#include "./service_tasks.h"
#include "./call_tasks.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "timers.h"
#include "./config.h"
#include <string.h>
#include "logging.h"

namespace gsm {
	volatile sim900::CardStatus card_status;
	volatile sim900::Registration registration;
	volatile uint8_t signal_strength;
}

static inline void reset_service_info() {
	gsm::card_status = sim900::CardStatus::ERROR;
	gsm::registration = sim900::Registration::ONGOING;
	gsm::signal_strength = 0;
}

static volatile uint8_t pending_tasks;

/** Used to poll card status, network registration, signal strength, etc.*/
static volatile TimerHandle_t module_state_timer;
static void module_status_timeout(TimerHandle_t timer);

/** If card fails, network connection lost(and SIM900 is not trying to establish it), etc. - starts
 * a delay before rebooting(if issue stays). */
static volatile TimerHandle_t connection_recovery_timer;
static void connection_recovery_timeout(TimerHandle_t timer);

static inline uint8_t to_int(gsm::Task task) {
	return (uint8_t)task;
}

static inline void schedule_module_state_update() {
	xTimerReset(module_state_timer, portMAX_DELAY);
}

static inline void stop_module_state_update() {
	xTimerStop(module_state_timer, portMAX_DELAY);
	atomic_clear(&pending_tasks, ~to_int(gsm::Task::CHECK_MODULE_STATE));
}

static inline void schedule_connection_recovery() {
	xTimerReset(connection_recovery_timer, portMAX_DELAY);
}

static inline void stop_connection_recovery() {
	xTimerStop(connection_recovery_timer, portMAX_DELAY);
}

void gsm::init_service_tasks() {
	pending_tasks = 0;
	reset_service_info();

	static StaticTimer_t module_status_timer_ctrl;
	module_state_timer = xTimerCreateStatic("gsm status", pdMS_TO_TICKS(MODULE_STATUS_UPDATE_PERIOD_ms),
											pdFALSE, (void *)0, module_status_timeout, &module_status_timer_ctrl);

	static StaticTimer_t timer_ctrl;
	connection_recovery_timer = xTimerCreateStatic("gsm conn", pdMS_TO_TICKS(CONNECTION_RECOVERY_TIME_ms),
												   pdFALSE, (void *) 0, connection_recovery_timeout, &timer_ctrl);
}

void gsm::schedule(Task task) {
	atomic_set(&pending_tasks, to_int(task));
}

void gsm::execute_or_schedule(gsm::Task task) {
	using namespace gsm;
	atomic_set(&pending_tasks, to_int(task));
	if (xSemaphoreTake(ctrl_mutex, 0) == pdTRUE) {
		execute_scheduled();
	}
}

void gsm::turn_module_on() {

}

static void turn_on();
static void turn_off();
static void check_module_state();

void gsm::execute_scheduled() {
	uint16_t pending_now = pending_tasks;

	if (pending_now & to_int(Task::REBOOT)) {
		atomic_clear(&pending_tasks, ~to_int(Task::REBOOT));
		atomic_set(&pending_tasks, to_int(Task::TURN_ON));
		pending_now |= to_int(Task::TURN_OFF);
	}

	if (pending_now & to_int(Task::TURN_OFF)) {
		turn_off();
	} else if (pending_now & to_int(Task::TURN_ON)) { // should be after "turn off" for reboot
		turn_on();
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

static void turn_on() {
	using namespace sim900;

	static constexpr auto turned_on = [](bool ok) {
		if (ok) {
			schedule_module_state_update();
		} else {
			rec::log("Failed to turn SIM900 on");
			schedule(Task::REBOOT);
		}

		executed(Task::TURN_ON);
	};

	sim900::turn_on(turned_on);
}

static void turn_off() {
	using namespace sim900;

	static constexpr auto turned_off = []() {
		stop_module_state_update();
		stop_connection_recovery();
		reset_service_info();
		terminate_calls();

		executed(Task::TURN_OFF);
	};

	sim900::turn_off(turned_off);
}

static void check_module_state() {
	using namespace sim900;

	static constexpr auto signal_checked = [](uint8_t signal_pct, Result res) {
		if (res == Result::OK) {
			gsm::signal_strength = signal_pct;
		}

		if (res == Result::OK || res == Result::ERROR) {
			schedule_module_state_update();
		} else {
			schedule(Task::REBOOT);
		}

		executed(Task::CHECK_MODULE_STATE);
	};

	static constexpr auto reg_checked = [](Registration new_reg, Result res) {
		bool check_next;

		if (res == Result::OK) {
			Registration prev_reg = gsm::registration;
			gsm::registration = new_reg;
			bool was_ok = prev_reg != Registration::FAILED;
			bool ok_now = new_reg != Registration::FAILED;
			if (was_ok && !ok_now) {
				schedule_connection_recovery();
			}
			check_next = new_reg == Registration::DONE;
		} else {
			check_next = false;
			if (res != Result::ERROR) {
				schedule(Task::REBOOT);
			}
		}

		if (check_next) {
			get_signal_strength(signal_checked);
		} else {
			schedule_module_state_update(); // schedules update even if going to reboot - ok, it's simpler
			executed(Task::CHECK_MODULE_STATE);
		}
	};

	static constexpr auto sim_checked = [](CardStatus new_status, Result res) {
		bool check_next;

		if (res == Result::OK) {
			CardStatus prev_status = gsm::card_status;
			gsm::card_status = new_status;
			bool was_ok = prev_status == CardStatus::READY;
			bool is_ok = new_status == CardStatus::READY;
			if (was_ok && !is_ok) {
				// card failure
				schedule_connection_recovery();
			}
			check_next = is_ok;
		} else {
			check_next = false;
			if (res != Result::ERROR) {
				schedule(Task::REBOOT);
			}
		}

		if (check_next) {
			get_registration(reg_checked);
		} else {
			schedule_module_state_update(); // schedules update even if going to reboot - ok, it's simpler
			executed(Task::CHECK_MODULE_STATE);
		}
	};

	sim900::get_card_status(sim_checked);
}

static void module_status_timeout(TimerHandle_t timer) {
	execute_or_schedule(Task::CHECK_MODULE_STATE);
}

static void connection_recovery_timeout(TimerHandle_t timer) {
	using namespace sim900;
	if (gsm::card_status != CardStatus::READY || gsm::registration == Registration::FAILED) {
		execute_or_schedule(Task::REBOOT);
	}
}