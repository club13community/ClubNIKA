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

/** Copies from src till '\0'(which is not copied).
 * @returns pointer to dst where tailing '\0'(or other END LINE) may be placed, or nullptr if not enough space. */
inline char * copy(const char * src, char * dst, unsigned int max_len) {
	char * limit = dst + max_len; // excluding
	while (*src != '\0') {
		if (dst < limit) {
			*dst++ = *src++;
		} else {
			return nullptr;
		}
	}
	return dst;
}

/** Copy part.
 * @param end - exclusive */
inline void copy(const char * src, unsigned int start, unsigned int end, char * dst) {
	src = src + start;
	unsigned int len = end - start;
	while (len-- > 0) {
		*dst++ = *src++;
	}
	*dst = '\0';
}

/** @returns length of '\0' terminated string(designed for literal) */
constexpr uint16_t length(const char * str) {
	const char * end = str;
	while (*end != '\0') {
		end++;
	}
	return end - str;
}