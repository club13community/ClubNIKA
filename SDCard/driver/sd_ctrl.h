//
// Created by independent-variable on 5/1/2024.
//

#pragma once
#include "sd_errors.h"

namespace sd {
	Error start_operation();
	void end_operation(Error error);
}
