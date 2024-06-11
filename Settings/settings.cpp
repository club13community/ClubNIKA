//
// Created by independent-variable on 5/8/2024.
//
#include "settings.h"
#include "ff.h"
#include "./SettingsDto.h"
#include "FreeRTOS.h"
#include "semphr.h"

#define CURRENT_PATH	"/flash/curr.cfg"
#define PREVIOUS_PATH	"/flash/prev.cfg"
#define TEMP_PATH		"/flash/temp.cfg"
static FIL file;
static SettingsDto settings[2];
static volatile uint8_t active = 0;
static volatile bool used_default, current_absent;
static SemaphoreHandle_t mutex;

static inline void take_mutex() {
	while (xSemaphoreTake(mutex, portMAX_DELAY) == pdFALSE);
}

static inline void give_mutex() {
	xSemaphoreGive(mutex);
}

void init_settings() {
	active = 0;
	set_defaults(settings[0]);
	used_default = true;
	current_absent = true; // actually any value
	static StaticSemaphore_t mutex_ctr;
	mutex = xSemaphoreCreateMutexStatic(&mutex_ctr);
	give_mutex();
}

bool load_settings() {
	take_mutex();
	FRESULT res;
	SettingsDto & next = settings[active ^ 0x01U];
	res = f_open(&file, CURRENT_PATH, FA_READ);
	current_absent = res == FR_NO_FILE;
	if (res == FR_OK || current_absent && f_open(&file, PREVIOUS_PATH, FA_READ) == FR_OK) {
		UINT read_bytes;
		res = f_read(&file, &next, sizeof (SettingsDto), &read_bytes);
		f_close(&file);
		used_default = res != FR_OK || read_bytes != sizeof (SettingsDto);
	} else {
		// load default
		used_default = true;
	}
	if (used_default) {
		set_defaults(next);
	}
	active ^= 0x01U;
	give_mutex();
	return !used_default;
}

static bool save_settings(SettingsDto & dto) {
	if (!current_absent) {
		// file with current config. exists
		// remove prev.
		FRESULT removal = f_unlink(PREVIOUS_PATH);
		if (removal != FR_OK && removal != FR_NO_FILE) {
			return false;
		}
		// rename current->prev.
		if (f_rename(CURRENT_PATH, PREVIOUS_PATH) != FR_OK) {
			return false;
		}
	}
	// create temp. config
	if (f_open(&file, TEMP_PATH, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
		return false;
	}
	// write to temp.
	UINT written;
	f_write(&file, &dto, sizeof (SettingsDto), &written);
	if (f_close(&file) != FR_OK || written != sizeof (SettingsDto)) {
		return false;
	}
	// rename temp to current
	if (f_rename(TEMP_PATH, CURRENT_PATH) != FR_OK) {
		return false;
	}
	return true;
}

static inline SettingsDto & prepare_next() {
	uint8_t active_now = active;
	uint8_t active_next = active_now ^ 0x01;
	settings[active_next] = settings[active_now];
	return settings[active_next];
}

static inline bool apply_next() {
	uint8_t next = active ^ 0x01U;
	if (save_settings(settings[next])) {
		active = next;
		return true;
	} else {
		return false;
	}
}

uint16_t get_alarm_delay_s() {
	// ok, without mutex
	return settings[active].alarm_delay_s;
}

bool set_alarm_delay_s(uint16_t delay) {
	bool applied = false;
	take_mutex();
	if (!used_default) {
		SettingsDto &next = prepare_next();
		next.alarm_delay_s = delay;
		applied = apply_next();
	}
	give_mutex();
	return applied;
}

bool is_correct_password(const char * pwd) {
	take_mutex();
	bool equals = used_default || settings[active].password_equals(pwd);
	give_mutex();
	return equals;
}

PasswordUpdate update_password(const char * curr_pwd, const char * new_pwd) {
	PasswordUpdate updated;
	take_mutex();
	if (used_default) {
		updated = PasswordUpdate::FAILED_TO_PERSIST;
	} else if (!settings[active].password_equals(curr_pwd)) {
		updated = PasswordUpdate::WRONG_PASSWORD;
	} else {
		SettingsDto & next = prepare_next();
		next.set_password(new_pwd);
		if (apply_next()) {
			updated = PasswordUpdate::DONE;
		} else {
			updated = PasswordUpdate::FAILED_TO_PERSIST;
		}
	}
	give_mutex();
	return updated;
}

uint8_t get_phones_count() {
	// ok without mutex, this method is for a single editor	of settings
	return settings[active].phones_count;
}

bool get_phone(uint8_t index, char * number) {
	take_mutex();
	bool copied = settings[active].get_phone(index, number);
	give_mutex();
	return copied;
}

bool set_phone(uint8_t index, const char * phone_number) {
	bool applied = false;
	take_mutex();
	if (!used_default) {
		SettingsDto & next = prepare_next();
		next.set_phone(index, phone_number);
		applied = apply_next();
	}
	give_mutex();
	return applied;
}

bool add_phone(const char * phone_number) {
	bool applied = false;
	take_mutex();
	if (!used_default) {
		SettingsDto & next = prepare_next();
		next.add_phone(phone_number);
		applied = apply_next();
	}
	give_mutex();
	return applied;
}

bool delete_phone(uint8_t index) {
	bool applied = false;
	take_mutex();
	if (!used_default) {
		SettingsDto &next = prepare_next();
		next.delete_phone(index);
		applied = apply_next();
	}
	give_mutex();
	return applied;
}

bool shift_phone(uint8_t old_index, uint8_t new_index) {
	bool applied = false;
	take_mutex();
	if (!used_default) {
		SettingsDto &next = prepare_next();
		next.shift_phone(old_index, new_index);
		applied = apply_next();
	}
	give_mutex();
	return applied;
}

bool is_known_phone(const char * number) {
	take_mutex();
	bool known = settings[active].contains_phone(number);
	give_mutex();
	return known;
}

ZoneActivationFlags get_zone_activations_for_isr() {
	SettingsDto & active_now = settings[active];
	return {.on_open = active_now.zones_active_on_open, .on_close = active_now.zones_active_on_close};
}

ZoneActivation get_zone_activation(uint8_t index) {
	take_mutex();
	ZoneActivation activation = settings[active].get_zone_activation(index);
	give_mutex();
	return activation;
}

bool set_zone_activation(uint8_t index, ZoneActivation activation) {
	bool applied = false;
	take_mutex();
	if (!used_default) {
		SettingsDto & next = prepare_next();
		next.set_zone_activation(index, activation);
		applied = apply_next();
	}
	give_mutex();
	return applied;
}

bool save_default_settings() {
	SettingsDto dto;
	set_defaults(dto);
	UINT written;
	return f_open(&file, CURRENT_PATH, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK
		   && f_write(&file, &dto, sizeof (SettingsDto), &written) == FR_OK
		   && f_close(&file) == FR_OK;
}