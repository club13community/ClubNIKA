//
// Created by independent-variable on 6/16/2024.
// Manages switching between power sources: socket or battery
//

#pragma once
#include "SupplySystem.h"

namespace supply {
	Source init_source();
	void source_exti_isr();
	void source_timer_isr();
}