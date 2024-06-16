//
// Created by independent-variable on 6/17/2024.
//

#pragma once
#include <stdint.h>

namespace supply {
	/** if > 0 - 12V supply outputs are enabled(not in over current protection state). */
	extern volatile uint8_t no_protection;
	/** if > 0 - system is supplied from socket(not battery). */
	extern volatile uint8_t socket_powered;
	/** Indicates if siren should be on, used when disabling over current protection. */
	extern volatile bool siren_on;
	/** Indicates if charging should be ongoing, used when switching to socket supply. */
	extern volatile bool charging;

}