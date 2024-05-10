//
// Created by independent-variable on 5/9/2024.
//

#pragma once

namespace alarm {
	// todo create wrapper which also notifies UI
	void arm();
	void disarm();
	bool is_armed();
}