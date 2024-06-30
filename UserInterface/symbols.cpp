//
// Created by independent-variable on 4/5/2024.
//
#include <stdint.h>
#include "./symbols.h"

namespace user_interface::symbol {

	const uint8_t battery[] = {
			0b01110,
			0b11111,
			0b10001,
			0b11111,
			0b10001,
			0b11111,
			0b10001,
			0b11111
	};

	const uint8_t socket[] = {
			0b00000,
			0b01010,
			0b01010,
			0b11111,
			0b10001,
			0b10001,
			0b01110,
			0b00000
	};

	const uint8_t charging_battery[] = {
			0b01110,
			0b11111,
			0b10001,
			0b10101,
			0b11111,
			0b10101,
			0b10001,
			0b11111
	};

	const uint8_t signal_level[] = {
			0b11111,
			0b11111,
			0b00000,
			0b01110,
			0b01110,
			0b00000,
			0b00100,
			0b00100
	};

	const uint8_t up[] = {
			0b00000,
			0b00100,
			0b01110,
			0b10101,
			0b00100,
			0b00100,
			0b00100,
			0b00000
	};

	const uint8_t down[] = {
			0b00000,
			0b00100,
			0b00100,
			0b00100,
			0b10101,
			0b01110,
			0b00100,
			0b00000
	};

	const uint8_t enter[] = {
			0b00000,
			0b00000,
			0b00101,
			0b01001,
			0b11111,
			0b01000,
			0b00100,
			0b00000
	};

	const uint8_t exit[] = {
			0b00100,
			0b01110,
			0b10101,
			0b00100,
			0b00100,
			0b11100,
			0b00000,
			0b00000
	};

	const uint8_t card[] = {
			0b00111,
			0b01001,
			0b10001,
			0b10001,
			0b10001,
			0b10001,
			0b11111,
			0b00000
	};

	const uint8_t heart[] = {
			0b00000,
			0b01010,
			0b10101,
			0b10001,
			0b10001,
			0b01010,
			0b00100,
			0b00000
	};
	/** U from Lupa */
	const uint8_t ua_U[] = {
			0b10001,
			0b10001,
			0b10011,
			0b10101,
			0b11001,
			0b10001,
			0b10001,
			0b00000
	};

	const uint8_t ua_P[] = {
			0b11111,
			0b10001,
			0b10001,
			0b10001,
			0b10001,
			0b10001,
			0b10001,
			0b00000
	};

	const uint8_t ua_F[] = {
			0b01110,
			0b10101,
			0b10101,
			0b10101,
			0b01110,
			0b00100,
			0b00100,
			0b00000
	};

	const uint8_t ua_L[] = {
			0b00111,
			0b01001,
			0b10001,
			0b10001,
			0b10001,
			0b10001,
			0b10001,
			0b00000
	};
	/**C from ceremonia */
	const uint8_t ua_C[] = {
			0b10010,
			0b10010,
			0b10010,
			0b10010,
			0b10010,
			0b11111,
			0b00001,
			0b00000
	};

	const uint8_t ua_MILD[] = {
			0b10000,
			0b10000,
			0b10000,
			0b11110,
			0b10001,
			0b10001,
			0b11110,
			0b00000
	};

	/** YU from Yula*/
	const uint8_t ua_YU[] = {
			0b10010,
			0b10101,
			0b10101,
			0b11101,
			0b10101,
			0b10101,
			0b10010,
			0b00000
	};

	const uint8_t ua_SH[] = {
			0b10101,
			0b10101,
			0b10101,
			0b10101,
			0b10101,
			0b10101,
			0b11111,
			0b00000
	};
	/** Y from Ydacha */
	const uint8_t ua_Y[] = {
			0b10001,
			0b10001,
			0b10001,
			0b01111,
			0b00001,
			0b10001,
			0b01110,
			0b00000
	};

	const uint8_t ua_D[] = {
			0b00110,
			0b01010,
			0b01010,
			0b01010,
			0b01010,
			0b11111,
			0b10001,
			0b00000
	};
	/** J from Jod */
	const uint8_t ua_J[] = {
			0b01110,
			0b00000,
			0b10001,
			0b10011,
			0b10101,
			0b11001,
			0b10001,
			0b00000
	};
	/** v from vira */
	const uint8_t ua_v[] = {
		0b00000,
		0b00000,
		0b11100,
		0b10100,
		0b11110,
		0b10010,
		0b11110,
		0b00000
	};

}