//
// Created by independent-variable on 5/15/2024.
//

#pragma once
#include <stdint.h>

/** Does not copy tailing '\0'.
 * @returns pointer to dst where tailing '\0'(or other END LINE) may be placed */
inline char * copy(const char * src, char * dst) {
	while (*src != '\0') {
		*dst++ = *src++;
	}
	return dst;
}

/** Copies from src till '\0', dst will be terminated by end.
 * @returns length of dst including end-character*/
inline uint16_t copy(const char * src, char * dst, char end) {
	char * endp = copy(src, dst);
	*endp++ = end;
	return endp - dst;
}

/** @returns length of '\0' terminated string(designed for literal) */
constexpr uint16_t length(const char * str) {
	const char * end = str;
	while (*end != '\0') {
		end++;
	}
	return end - str;
}