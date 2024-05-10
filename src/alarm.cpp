//
// Created by independent-variable on 5/9/2024.
//
#include "alarm.h"

static volatile bool armed = false;

void alarm::arm() {
	armed = true;
}

void alarm::disarm() {
	armed = false;
}

bool alarm::is_armed() {
	return armed;
}