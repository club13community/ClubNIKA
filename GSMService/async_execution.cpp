//
// Created by independent-variable on 5/26/2024.
//
#include "./async_execution.h"
#include "./service.h"
#include "concurrent_utils.h"

namespace gsm {
	static volatile uint8_t pending_tasks = 0;

	static inline void set_pending(Task task) {
		atomic_set(&pending_tasks, (uint8_t) task);
	}

	void clear_pending(Task task) {
		atomic_clear(&pending_tasks, ~(uint8_t) task);
	}

	static inline uint8_t flag_of(Task task) {
		return (uint8_t) task;
	}

	void execute_scheduled() {
		uint8_t pending_now = pending_tasks;

		if (pending_now & flag_of(Task::REBOOT)) {
			clear_pending(Task::REBOOT);
			set_pending(Task::TURN_ON);
			pending_now |= flag_of(Task::TURN_OFF);
		}

		if (pending_now & flag_of(Task::TURN_OFF)) {
			turn_off();
		} else if (pending_now & flag_of(Task::TURN_ON)) { // should be after "turn off" for reboot
			turn_on();
		} else if (pending_now & flag_of(Task::CHECK_MODULE_STATE)) {
			check_module_state();
		} else {
			xSemaphoreGive(ctrl_mutex);
		}
	}

	inline void execute_or_schedule(Task task) {
		set_pending(task);
		if (xSemaphoreTake(ctrl_mutex, 0) == pdTRUE) {
			execute_scheduled();
		}
	}

	void executed(Task task) {
		clear_pending(task);
		gsm::execute_scheduled();
	}

	void schedule_reboot() {
		set_pending(Task::REBOOT);
	}

	void reboot_asap() {
		execute_or_schedule(Task::REBOOT);
	}

	void check_module_state_asap() {
		execute_or_schedule(Task::CHECK_MODULE_STATE);
	}
}