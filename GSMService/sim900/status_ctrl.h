//
// Created by independent-variable on 5/14/2024.
//

#pragma once
#include "sim900.h"

namespace sim900 {
	bool get_signal_strength(SignalHandler handler);
	bool get_registration(RegistrationHandler handler);
}