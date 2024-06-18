//
// Created by independent-variable on 6/16/2024.
//

#pragma once

namespace supply {
	/** Initializes fuse in non-conductive state. */
	void init_fuse();
	void fuse_exti_isr();
	void fuse_timer_isr();
}