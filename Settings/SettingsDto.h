//
// Created by independent-variable on 5/8/2024.
//

#pragma once
#include "settings.h"
#include <stdint.h>
#include <string.h>

struct SettingsDto {
	uint16_t alarm_delay_s;
	char password[PASSWORD_LENGTH];
	uint8_t phones_count;
	char phones[MAX_PHONE_NUMBERS][MAX_PHONE_LENGTH + 1];
	uint8_t zones_active_on_open;
	uint8_t zones_active_on_close;

	bool password_equals(const char * pwd) {
		for (uint8_t i = 0; i < PASSWORD_LENGTH; i++) {
			if (pwd[i] != password[i]) {
				return false;
			}
		}
		return true;
	}

	void set_password(const char * pwd) {
		for (uint8_t i = 0; i < PASSWORD_LENGTH; i++) {
			password[i] = pwd[i];
		}
	}

	bool contains_phone(const char * number) {
		for (uint8_t i = 0; i < phones_count; i++) {
			if (strcmp(number, phones[i]) == 0) {
				return true;
			}
		}
		return false;
	}

	bool get_phone(uint8_t index, char * number) {
		if (index >= phones_count) {
			return false;
		}
		strcpy(number, phones[index]);
		return true;
	}

	void set_phone(uint8_t index, const char * number) {
		strcpy(phones[index], number);
	}

	void add_phone(const char * new_number) {
		strcpy(phones[phones_count++], new_number);
	}

	void delete_phone(uint8_t index) {
		for (uint8_t dst = index, src = index + 1; src < phones_count; dst++, src++) {
			strcpy(phones[dst], phones[src]);
		}
		phones_count -= 1;
	}

	void shift_phone(uint8_t old_index, uint8_t new_index) {
		// shift phones between old and new indexes
		char phone[MAX_PHONE_LENGTH + 1];
		strcpy(phone, phones[old_index]);
		int8_t step = new_index < old_index ? -1 : 1;
		uint8_t dst = old_index, src = old_index + step;
		while (dst != new_index) {
			strcpy(phones[dst], phones[src]);
			dst += step;
			src += step;
		}
		// put phone in new position
		strcpy(phones[new_index], phone);
	}

	ZoneActivation get_zone_activation(uint8_t index) {
		uint8_t mask = 1U << index;
		if (zones_active_on_open & mask) {
			return ZoneActivation::ON_OPEN;
		} else if (zones_active_on_close & mask) {
			return ZoneActivation::ON_CLOSE;
		} else {
			return ZoneActivation::NEVER;
		}
	}

	void set_zone_activation(uint8_t index, ZoneActivation activation) {
		uint8_t mask = 1U << index;
		if (activation == ZoneActivation::ON_OPEN) {
			zones_active_on_open |= mask;
			zones_active_on_close &= ~mask;
		} else if (activation == ZoneActivation::ON_CLOSE) {
			zones_active_on_open &= ~mask;
			zones_active_on_close |= mask;
		} else {
			zones_active_on_open &= ~mask;
			zones_active_on_close &= ~mask;
		}
	}
};

inline void set_defaults(SettingsDto & dto) {
	dto.alarm_delay_s = 60;
	for (uint8_t i = 0; i < PASSWORD_LENGTH; i++) {
		dto.password[i] = '0';
	}
	dto.phones_count = 0;
	dto.zones_active_on_open = 0xFFU;
	dto.zones_active_on_close = 0;
}