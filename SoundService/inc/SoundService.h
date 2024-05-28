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

namespace player {
	void start();
	bool play_via_speaker(const char * file, void (* finished)());
	bool play_for_gsm(const char * file, void (* finished)());
	void stop_play();
	bool is_playing();
}

