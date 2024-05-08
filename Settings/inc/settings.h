//
// Created by independent-variable on 5/8/2024.
//

#pragma once
#include <stdint.h>

/** Min. length of phone number */
#define MIN_PHONE_LENGTH	3U
/** Max. length of phone number */
#define MAX_PHONE_LENGTH	11U
/** Max. count of phone numbers */
#define MAX_PHONE_NUMBERS	8U

#define PASSWORD_LENGTH		4U
/** Number of security zones */
#define ZONES_COUNT			8

enum class ZoneActivation : uint8_t {
	/** Newer trigger alarm */
	NEVER = 0,
	/** Sensor breaking(not conducting current) triggers alarm */
	ON_OPEN,
	/** Sensor shorting(conducting current) triggers alarm */
	ON_CLOSE
};

inline ZoneActivation next_value(ZoneActivation value) {
	uint8_t next = (uint8_t)value + 1;
	return next <= (uint8_t)ZoneActivation::ON_CLOSE ? (ZoneActivation)next : ZoneActivation::NEVER;
}

enum class PasswordUpdate : uint8_t  {
	DONE,
	/** Was not updated, because of wrong current passw. */
	WRONG_PASSWORD,
	/** Error during saving new passw. */
	FAILED_TO_PERSIST
};

/** Loads persisted settings.
 * @returns true if successfully */
bool load_settings();

uint16_t get_alarm_delay_s();
/** @returns false if failed to persist new delay */
bool set_alarm_delay_s(uint16_t delay);

/** @returns true if password is correct or default settings are applied */
bool is_correct_password(const char * pwd);
PasswordUpdate update_password(const char * curr_pwd, const char * new_pwd);

uint8_t get_phones_count();
const char * get_phone(uint8_t index);
const char (* get_phones())[MAX_PHONE_LENGTH + 1];
/** @returns false if failed to store */
bool set_phone(uint8_t index, const char * phone_number);
bool add_phone(const char * phone_number);
bool delete_phone(uint8_t index);
bool shift_phone(uint8_t old_index, uint8_t new_index);

/** @returns pointer to array with 8 items */
const ZoneActivation * get_zone_activations();
/** @returns false if failed to save */
bool set_zone_activation(uint8_t index, ZoneActivation activation);

/** Use to initialize */
bool save_default_settings();