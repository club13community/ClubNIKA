//
// Created by independent-variable on 5/8/2024.
//

#pragma once
#include "settings.h"
#include <stdint.h>

struct SettingsDto {
	uint16_t alarm_delay_s;
	char password[PASSWORD_LENGTH];
	uint8_t phones_count;
	char phones[MAX_PHONE_NUMBERS][MAX_PHONE_LENGTH + 1];
	ZoneActivation zone_activation[8];
};

inline void set_defaults(SettingsDto & dto) {
	dto.alarm_delay_s = 60;
	for (uint8_t i = 0; i < PASSWORD_LENGTH; i++) {
		dto.password[i] = '0';
	}
	dto.phones_count = 0;
	for (uint8_t i = 0; i < 8; i++) {
		dto.zone_activation[i] = ZoneActivation::ON_OPEN;
	}
}

inline bool equal_passwords(char * pwd, SettingsDto & dto) {
	for (uint8_t i = 0; i < PASSWORD_LENGTH; i++) {
		if (pwd[i] != dto.password[i]) {
			return false;
		}
	}
	return true;
}

inline void set_password(char * pwd, SettingsDto & dto) {
	for (uint8_t i = 0; i < PASSWORD_LENGTH; i++) {
		dto.password[i] = pwd[i];
	}
}