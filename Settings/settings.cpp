//
// Created by independent-variable on 5/8/2024.
//
#include "settings.h"
#include "ff.h"
#include <string.h>
#include "./SettingsDto.h"

#define CURRENT_PATH	"/flash/curr.cfg"
#define PREVIOUS_PATH	"/flash/prev.cfg"
#define TEMP_PATH		"/flash/temp.cfg"
static FIL file;
static SettingsDto settings;
static bool used_default, current_absent;

bool load_settings() {
	FRESULT res;
	used_default = false;
	res = f_open(&file, CURRENT_PATH, FA_READ);
	current_absent = res == FR_NO_FILE;
	if (res == FR_OK || current_absent && f_open(&file, PREVIOUS_PATH, FA_READ) == FR_OK) {
		UINT read_bytes;
		res = f_read(&file, &settings, sizeof (settings), &read_bytes);
		if (res != FR_OK) {
			// failed to read - load default
			used_default = true;
			set_defaults(settings);
		}
		f_close(&file);
	} else {
		// load default
		used_default = true;
		set_defaults(settings);
	}
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

uint16_t get_alarm_delay_s() {
	return settings.alarm_delay_s;
}

bool set_alarm_delay_s(uint16_t delay) {
	if (used_default) {
		return false;
	}
	SettingsDto temp = settings;
	temp.alarm_delay_s = delay;
	if (save_settings(temp)) {
		settings.alarm_delay_s = delay;
		return true;
	} else {
		return false;
	}
}

bool is_correct_password(const char * pwd) {
	if (used_default) {
		return true;
	}
	for (uint8_t i = 0; i < PASSWORD_LENGTH; i++) {
		if (pwd[i] != settings.password[i]) {
			return false;
		}
	}
	return true;
}

PasswordUpdate update_password(const char * curr_pwd, const char * new_pwd) {
	if (used_default) {
		return PasswordUpdate::FAILED_TO_PERSIST;
	}
	if (is_correct_password(curr_pwd)) {
		return PasswordUpdate::WRONG_PASSWORD;
	}
	SettingsDto temp = settings;
	set_password(new_pwd, temp);
	if (save_settings(temp)) {
		set_password(new_pwd, settings);
		return PasswordUpdate::DONE;
	} else {
		return PasswordUpdate::FAILED_TO_PERSIST;
	}
}

uint8_t get_phones_count() {
	return settings.phones_count;
}

const char * get_phone(uint8_t index) {
	return settings.phones[index];
}

const char (* get_phones())[MAX_PHONE_LENGTH + 1] {
	return settings.phones;
}

bool set_phone(uint8_t index, const char * phone_number) {
	if (used_default) {
		return false;
	}
	SettingsDto temp = settings;
	strcpy(temp.phones[index], phone_number);
	if (save_settings(temp)) {
		strcpy(settings.phones[index], phone_number);
		return true;
	} else {
		return false;
	}
}

bool add_phone(const char * phone_number) {
	if (used_default) {
		return false;
	}
	SettingsDto temp = settings;
	add_phone(phone_number, temp);
	if (save_settings(temp)) {
		add_phone(phone_number, settings);
		return true;
	} else {
		return false;
	}
}

bool delete_phone(uint8_t index) {
	if (used_default) {
		return false;
	}
	SettingsDto temp = settings;
	delete_phone(index, temp);
	if (save_settings(temp)) {
		delete_phone(index, settings);
		return true;
	} else {
		return false;
	}
}

bool shift_phone(uint8_t old_index, uint8_t new_index) {
	if (used_default) {
		return false;
	}
	SettingsDto temp = settings;
	shift_phone(old_index, new_index, temp);
	if (save_settings(temp)) {
		shift_phone(old_index, new_index, settings);
		return true;
	} else {
		return false;
	}
}

const ZoneActivation * get_zone_activations() {
	return settings.zone_activation;
}

bool set_zone_activation(uint8_t index, ZoneActivation activation) {
	if (used_default) {
		return false;
	}
	SettingsDto temp = settings;
	temp.zone_activation[index] = activation;
	if (save_settings(temp)) {
		settings.zone_activation[index] = activation;
		return true;
	} else {
		return false;
	}
}

bool save_default_settings() {
	SettingsDto dto;
	set_defaults(dto);
	UINT written;
	return f_open(&file, CURRENT_PATH, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK
		   && f_write(&file, &dto, sizeof (SettingsDto), &written) == FR_OK
		   && f_close(&file) == FR_OK;
}