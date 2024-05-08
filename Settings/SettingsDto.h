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

inline void set_password(const char * pwd, SettingsDto & dto) {
	for (uint8_t i = 0; i < PASSWORD_LENGTH; i++) {
		dto.password[i] = pwd[i];
	}
}

inline void add_phone(const char * phone_number, SettingsDto & dto) {
	strcpy(dto.phones[dto.phones_count++], phone_number);
}

inline void delete_phone(uint8_t index, SettingsDto & dto) {
	for (uint8_t dst = index, src = index + 1; src < dto.phones_count; dst++, src++) {
		strcpy(dto.phones[dst], dto.phones[src]);
	}
	dto.phones_count -= 1;
}

inline void shift_phone(uint8_t old_index, uint8_t new_index, SettingsDto & dto) {
	// shift phones between old and new indexes
	char phone[MAX_PHONE_LENGTH + 1];
	strcpy(phone, dto.phones[old_index]);
	int8_t step = new_index < old_index ? -1 : 1;
	uint8_t dst = old_index, src = old_index + step;
	while (dst != new_index) {
		strcpy(dto.phones[dst], dto.phones[src]);
		dst += step;
		src += step;
	}
	// put phone in new position
	strcpy(dto.phones[new_index], phone);
}