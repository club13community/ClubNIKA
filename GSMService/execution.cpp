//
// Created by independent-variable on 5/26/2024.
//
#include "./execution.h"
#include "./service.h"
#include "./sms.h"
#include "concurrent_utils.h"

namespace gsm {
	static volatile uint8_t pending_tasks = 0;

	static inline void set_pending(AsyncTask task) {
		atomic_set(&pending_tasks, (uint8_t) task);
	}

	void clear_pending(AsyncTask task) {
		atomic_clear(&pending_tasks, ~(uint8_t) task);
	}

	static inline uint8_t flag_of(AsyncTask task) {
		return (uint8_t) task;
	}

	void executed() {
		uint8_t pending_now = pending_tasks;

		if (pending_now & flag_of(AsyncTask::REBOOT)) {
			clear_pending(AsyncTask::REBOOT);
			set_pending(AsyncTask::TURN_ON);
			pending_now |= flag_of(AsyncTask::TURN_OFF);
		}

		if (pending_now & flag_of(AsyncTask::TURN_OFF)) {
			turn_off();
		} else if (pending_now & flag_of(AsyncTask::TURN_ON)) { // should be after "turn off" for reboot
			turn_on();
		} else if (pending_now & flag_of(AsyncTask::CHECK_MODULE_STATE)) {
			check_module_state();
		} else if (pending_now & flag_of(AsyncTask::DELETE_INCOMING_SMS)) {
			delete_incoming_sms();
		} else {
			xSemaphoreGive(ctrl_mutex);
		}
	}

	inline void execute_or_schedule(AsyncTask task) {
		set_pending(task);
		if (xSemaphoreTake(ctrl_mutex, 0) == pdTRUE) {
			executed();
		}
	}

	void executed(AsyncTask task) {
		clear_pending(task);
		gsm::executed();
	}

	void schedule_reboot() {
		set_pending(AsyncTask::REBOOT);
	}

	void reboot_asap() {
		execute_or_schedule(AsyncTask::REBOOT);
	}

	void check_module_state_asap() {
		execute_or_schedule(AsyncTask::CHECK_MODULE_STATE);
	}

	void delete_incoming_sms_asap() {
		execute_or_schedule(AsyncTask::DELETE_INCOMING_SMS);
	}
}