//
// Created by independent-variable on 3/20/2024.
//
#include "string_utils.h"

bool equals(const char * a, const char * b, uint16_t len) {
	while (len >= 4) {
		if (*(uint32_t *)a != *(uint32_t *)b) {
			return false;
		}
		a += 4;
		b += 4;
		len -= 4;
	}
	if (len >= 2) {
		if (*(uint16_t *)a != *(uint16_t *)b) {
			return false;
		}
		a += 2;
		b += 2;
		len -= 2;
	}
	if (len > 0 && *a != *b) {
		return false;
	}
	return true;
}