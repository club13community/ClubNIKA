//
// Created by independent-variable on 5/9/2024.
//

#pragma once
#include "ff.h"

namespace alarm {
	void start();
	void arm();
	void disarm();
	bool is_armed();
	/** Copies all needed wav files from SD card to flash memory.
	 * Use only for initialization of flash. Uses > 128 bytes of stack.
	 * src and dst are buffers for file-objects. */
	FRESULT copy_wav_to_flash(FIL * src, FIL * dst);
}