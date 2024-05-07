//
// Created by independent-variable on 3/17/2024.
//
#include <exception>
#include "sim900_driver.h"
#include "./sim900_power_ctrl.h"
#include "./sim900_uart_ctrl.h"

void sim900::config_peripherals() {
	using namespace sim900;
	init_power_ctrl();
	init_uart_ctrl();
}