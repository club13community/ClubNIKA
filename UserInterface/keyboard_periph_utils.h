//
// Created by independent-variable on 3/21/2024.
//

#pragma once
#include "stm32f10x.h"
#include "concurrent_utils.h"

namespace keyboard {
	inline void select_row_0() {
		GPIO_ResetBits(GPIOC, GPIO_Pin_6);
	}

	inline void select_row_1() {
		GPIO_ResetBits(GPIOC, GPIO_Pin_7);
	}

	inline void select_row_2() {
		GPIO_ResetBits(GPIOD, GPIO_Pin_10);
	}

	inline void select_row_3() {
		GPIO_ResetBits(GPIOD, GPIO_Pin_11);
	}

	inline void deselect_row_0() {
		GPIO_SetBits(GPIOC, GPIO_Pin_6);
	}

	inline void deselect_row_1() {
		GPIO_SetBits(GPIOC, GPIO_Pin_7);
	}

	inline void deselect_row_2() {
		GPIO_SetBits(GPIOD, GPIO_Pin_10);
	}

	inline void deselect_row_3() {
		GPIO_SetBits(GPIOD, GPIO_Pin_11);
	}

	inline void select_all_rows() {
		select_row_0();
		select_row_1();
		select_row_2();
		select_row_3();
	}

	inline void deselect_all_rows() {
		deselect_row_0();
		deselect_row_1();
		deselect_row_2();
		deselect_row_3();
	}

	inline void select_row(uint8_t row) {
		switch (row) {
			case 0:
				select_row_0();
				break;
			case 1:
				select_row_1();
				break;
			case 2:
				select_row_2();
				break;
			case 3:
				select_row_3();
				break;
		}
	}

	inline bool is_pressed(uint8_t col) {
		return !(GPIO_ReadInputData(GPIOD) & 1U << (12 + col));
	}

	inline uint8_t read_columns() {
		//PD12-PD15 - C0-C3
		uint16_t value = ~GPIO_ReadInputData(GPIOD);
		return (value >> 12); // bits [15..4] are already 0
	}

	inline void enable_exti_for(uint8_t col) {
		uint32_t line = 1U << (12 + col);
		atomic_set(&EXTI->IMR, line);
	}

	inline void enable_exti_for_all() {
		atomic_set(&EXTI->IMR, EXTI_IMR_MR12 | EXTI_IMR_MR13 | EXTI_IMR_MR14 | EXTI_IMR_MR15);
	}

	/** Clears EXTI and NVIC pending flags - keyboard will not be invoked after this method returns. */
	inline void disable_exti_for_all() {
		atomic_clear(&EXTI->IMR, ~(EXTI_IMR_MR12 | EXTI_IMR_MR13 | EXTI_IMR_MR14 | EXTI_IMR_MR15));
		EXTI->PR = EXTI_PR_PR12 | EXTI_PR_PR13 | EXTI_PR_PR14 | EXTI_PR_PR15;
		NVIC_ClearPendingIRQ(EXTI15_10_IRQn); // in case it was invoked from more priority ISR
	}

	inline void exti_on_release_for(uint8_t col) {
		uint32_t line = 1U << (12 + col);
		atomic_clear(&EXTI->FTSR, ~line);
		atomic_set(&EXTI->RTSR, line);
	}

	inline void exti_on_press_for_all() {
		atomic_clear(&EXTI->RTSR, ~(EXTI_RTSR_TR12 | EXTI_RTSR_TR13 | EXTI_RTSR_TR14 | EXTI_RTSR_TR15));
		atomic_set(&EXTI->FTSR, EXTI_RTSR_TR12 | EXTI_RTSR_TR13 | EXTI_RTSR_TR14 | EXTI_RTSR_TR15);
	}

	inline void set_exti_flag_for(uint8_t col) {
		uint32_t line = 1U << (12 + col);
		EXTI->SWIER = line;
	}

	inline void clear_exti_flag_for(uint8_t col) {
		uint32_t line = 1U << (12 + col);
		EXTI->PR = line;
	}

	inline void clear_exti_flags_for_all() {
		EXTI->PR = EXTI_PR_PR12 | EXTI_PR_PR13 | EXTI_PR_PR14 | EXTI_PR_PR15;
	}
}