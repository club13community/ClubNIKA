/*
 * SupplySystem.h
 *
 *  Created on: 30 ���. 2020 �.
 *      Author: MaxCm
 */

#pragma once
#include <stdint.h>

namespace supply {
	enum class Source : uint8_t {SOCKET = 0, BATTERY};

	void init();
	void start();
	void turn_on_siren();
	void turn_off_siren();
	Source get_source();
	uint16_t get_battery_mV();
	uint8_t get_battery_pct();

	void exti_isr();
	void timer_isr();
}