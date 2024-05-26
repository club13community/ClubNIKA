//
// Created by independent-variable on 5/21/2024.
//

#pragma once
#include "sim900.h"
#include <stdint.h>

namespace gsm {
	void init_service_tasks();
	void turn_on();
	void turn_off();
	void check_module_state();
}