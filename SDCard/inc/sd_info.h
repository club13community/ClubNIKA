//
// Created by independent-variable on 4/26/2024.
//

#pragma once
#include <stdint.h>

namespace sd {
	uint32_t get_capacity_kb();
	uint32_t get_capacity_mb();
	uint32_t get_capacity_gb();
	bool is_write_protected();
}