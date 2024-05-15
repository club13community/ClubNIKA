//
// Created by independent-variable on 5/14/2024.
// Everything needed for command execution
//

#pragma once
#include "GSMService.h"

namespace gsm {
	void set_end(Handler callback);
	void end_handler();
}