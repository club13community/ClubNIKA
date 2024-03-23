//
// Created by independent-variable on 3/22/2024.
//

#pragma once
#include <stdint.h>
#include <core_cm3.h>

inline void atomic_set(volatile uint16_t * val_ptr, uint16_t set_msk) {
	uint16_t val;
	do {
		val = __LDREXH(const_cast<uint16_t *>(val_ptr));
		val = val | set_msk;
	} while (__STREXH(val, const_cast<uint16_t *>(val_ptr)));
}

inline void atomic_clear(volatile uint16_t * val_ptr, uint16_t clr_msk) {
	uint16_t val;
	do {
		val = __LDREXH(const_cast<uint16_t *>(val_ptr));
		val = val & clr_msk;
	} while (__STREXH(val, const_cast<uint16_t *>(val_ptr)));
}

inline void atomic_modify(volatile uint16_t * val_ptr, uint16_t clr_msk, uint16_t set_msk) {
	uint16_t val;
	do {
		val = __LDREXH(const_cast<uint16_t *>(val_ptr));
		val = val & clr_msk | set_msk;
	} while (__STREXH(val, const_cast<uint16_t *>(val_ptr)));
}

inline void atomic_set(volatile uint32_t * val_ptr, uint32_t set_msk) {
	uint32_t val;
	do {
		val = __LDREXW(const_cast<uint32_t *>(val_ptr));
		val = val | set_msk;
	} while (__STREXW(val, const_cast<uint32_t *>(val_ptr)));
}

inline void atomic_clear(volatile uint32_t * val_ptr, uint32_t clr_msk) {
	uint32_t val;
	do {
		val = __LDREXW(const_cast<uint32_t *>(val_ptr));
		val = val & clr_msk;
	} while (__STREXW(val, const_cast<uint32_t *>(val_ptr)));
}

inline void atomic_modify(volatile uint32_t * val_ptr, uint32_t clr_msk, uint32_t set_msk) {
	uint32_t val;
	do {
		val = __LDREXW(const_cast<uint32_t *>(val_ptr));
		val = val & clr_msk | set_msk;
	} while (__STREXW(val, const_cast<uint32_t *>(val_ptr)));
}