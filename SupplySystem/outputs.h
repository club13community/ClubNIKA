//
// Created by independent-variable on 6/17/2024.
//

#pragma once

namespace supply {
	/** Initializes 12V outputs in non-conductive state. */
	void init_outputs();
	/** @param applied true if 12V outputs should be shut-downed. */
	void output_protection_changed(bool applied);
}