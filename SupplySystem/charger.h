//
// Created by independent-variable on 6/16/2024.
//

#pragma once
#include <stdint.h>
#include "SupplySystem.h"

namespace supply {
	void init_charger(Source source);
	void charger_source_changed(Source new_source);
	void charger_battery_measured(uint16_t bat_mV);
}