/*
 * WiredSensorMonitor.h
 *
 *  Created on: 3 квіт. 2020 р.
 *      Author: MaxCm
 */

#pragma once

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

namespace wired_zones {
	void init_periph();
	void start();
	uint8_t get_active();
}
