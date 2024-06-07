//
// Created by independent-variable on 5/7/2024.
//

#pragma once
#include <stdint.h>

namespace rec {
	void init_card_recorder();
	void write_to_card(const char * message, uint16_t len);
	void flush_to_card();
}
