/*
 * SoundService.h
 *
 *  Created on: 3 квіт. 2020 р.
 *      Author: MaxCm
 */

#pragma once

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"

namespace sound_service {
	void init_periph();
}

