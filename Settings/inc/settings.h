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

enum class ZoneActivation : uint8_t {
	/** Newer trigger alarm */
	NEVER = 0,
	/** Sensor breaking(not conducting current) triggers alarm */
	ON_OPEN,
	/** Sensor shorting(conducting current) triggers alarm */
	ON_CLOSE
};

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

/** @returns false */
PasswordUpdate update_password(char * curr_pwd, char * new_pwd);

uint8_t get_phones_count();
const char * get_phone(uint8_t index);
/** @returns false if failed to store */
bool set_phone(uint8_t index, char * phone_number);

/** @returns pointer to array with 8 items */
const ZoneActivation * get_zone_activations();
/** @returns false if failed to save */
bool set_zone_activation(uint8_t index, ZoneActivation activation);

/** Use to initialize */
bool save_default_settings();