//
// Created by independent-variable on 6/29/2024.
//

#pragma once
#include <stdint.h>
#include "stm32f10x.h"
#include "periph_allocation.h"

inline uint32_t dis_irq() {
	uint32_t basepri = __get_BASEPRI();
	__set_BASEPRI(LAST_MASKABLE_IRQ_PRIORITY << (8 - __NVIC_PRIO_BITS));
	return basepri;
}

inline void en_irq(uint32_t basepri) {
	__set_BASEPRI(basepri);
}