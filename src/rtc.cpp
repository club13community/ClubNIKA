//
// Created by independent-variable on 5/19/2024.
//
#include "rtc.h"

char * rtc::Timestamp::to_string(char * buf) const {
	char * tail = date.to_string(buf);
	*tail++ = ' ';
	tail = time.to_string(tail);
	return tail;
}