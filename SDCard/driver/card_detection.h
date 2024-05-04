//
// Created by independent-variable on 5/1/2024.
//

#pragma once
#include "sd_errors.h"

namespace sd {
	void wait_insertion(void (* on_inserted)());
	void check_presence(void (* on_absent)());
	void wait_removal(void (* on_removed)());
}