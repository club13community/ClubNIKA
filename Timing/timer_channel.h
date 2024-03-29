//
// Created by independent-variable on 3/28/2024.
//

#pragma once
#include "timing.h"
#include <stdint.h>
#include "stm32f10x.h"

namespace timing {
	typedef void (* SetterOfCCR)(TIM_TypeDef *, uint16_t);

	template<SetterOfCCR set_CCRx, uint16_t TIM_SR_CCxIF, uint16_t TIM_DIER_CCxIE>
	class TimerChannel {
	private:
		enum OnTimeout {
			START_SINGLE, START_REPETITIVE, START_WAIT, STOP, REPEAT, STOP_WAIT
		};

		struct State {
			Callback callback;
			uint16_t period;
			/** what is executed at the end of ISR(stop channel, schedule_timeout next invocation, etc.) */
			OnTimeout action;
		};

		TIM_TypeDef * const tim;
		volatile State state;
		volatile State new_state;
		volatile bool update_state;
		volatile bool wait_done;

		inline bool is_serving_timeout() {
			return tim->DIER & TIM_DIER_CCxIE && tim->SR & TIM_SR_CCxIF;
		}

		inline uint32_t mask_irq() {
			uint32_t primask = __get_PRIMASK();
			__set_PRIMASK(1U);
			return primask;
		}

		inline void unmask_irq(uint32_t prev_primask) {
			__set_PRIMASK(prev_primask);
		}

		inline void set_state(Callback callback, uint16_t period, OnTimeout action) {
			state.callback = callback;
			state.period = period;
			state.action = action;
		}

		inline void request_state(Callback callback, uint16_t period, OnTimeout action) {
			new_state.callback = callback;
			new_state.period = period;
			new_state.action = action;
			update_state = true;
		}

		inline void enable_timeout_irq() {
			tim->DIER |= TIM_DIER_CCxIE;
		}

		inline void disable_timeout_irq() {
			tim->DIER &= ~TIM_DIER_CCxIE;
		}

		inline void schedule_timeout(uint16_t ticks) {
			tim->CR1 &= ~TIM_CR1_CEN;
			set_CCRx(tim, tim->CNT + ticks);
			tim->SR = ~TIM_SR_CCxIF;
			tim->CR1 |= TIM_CR1_CEN;
		}

	protected:
		TimerChannel(TIM_TypeDef * timer) : tim(timer) {}

		inline void invoke_callback() {
			Callback callback = state.callback;
			if (callback != nullptr) {
				callback();
			}
		}

		/** Invoked at the end of ISR, interrupts are disabled, counter is stopped.
		 * Interrupts and counter will be enabled, interrupt flag will be cleared by ISR after this method returns */
		inline void handle_timeout() {
			OnTimeout action;
			if (update_state) {
				action = new_state.action;
				state.callback = new_state.callback;
				state.period = new_state.period;
				state.action = action;
				update_state = false;
			} else {
				action = state.action;
			}
			switch (action) {
				case START_SINGLE:
					set_CCRx(tim, tim->CNT + state.period);
					tim->DIER |= TIM_DIER_CCxIE;
					state.action = STOP;
					break;
				case START_REPETITIVE:
					set_CCRx(tim, tim->CNT + state.period);
					tim->DIER |= TIM_DIER_CCxIE;
					state.action = REPEAT;
					break;
				case START_WAIT:
					set_CCRx(tim, tim->CNT + state.period);
					tim->DIER |= TIM_DIER_CCxIE;
					state.action = STOP_WAIT;
					break;
				case STOP:
					tim->DIER &= ~TIM_DIER_CCxIE;
					break;
				case REPEAT:
					set_CCRx(tim, tim->CNT + state.period);
					break;
				case STOP_WAIT:
					wait_done = true;
					tim->DIER &= ~TIM_DIER_CCxIE;
					break;
			}
		}

		inline void stop() {
			uint32_t prev_mask = mask_irq();
			if (is_serving_timeout()) {
				request_state(nullptr, 0, STOP);
			} else {
				disable_timeout_irq();
			}
			unmask_irq(prev_mask);
		}

		inline void invoke_in_ticks(uint16_t ticks, Callback callback) {
			uint16_t period = ticks + 1;
			uint32_t prev_mask = mask_irq();
			if (is_serving_timeout()) {
				request_state(callback, period, START_SINGLE);
			} else {
				set_state(callback, period, STOP);
				schedule_timeout(period);
				enable_timeout_irq();
			}
			unmask_irq(prev_mask);
		}

		inline void every_ticks_invoke(uint16_t ticks, Callback callback) {
			uint16_t period = ticks + 1;
			uint32_t prev_mask = mask_irq();
			if (is_serving_timeout()) {
				request_state(callback, period, START_REPETITIVE);
			} else {
				set_state(callback, period, REPEAT);
				schedule_timeout(period);
				enable_timeout_irq();
			}
			unmask_irq(prev_mask);
		}

		inline void wait_ticks(uint16_t ticks) {
			wait_done = false;
			uint32_t period = ticks + 1;
			uint32_t prev_mask = mask_irq();
			if (is_serving_timeout()) {
				request_state(nullptr, period, START_WAIT);
			} else {
				set_state(nullptr, period, STOP_WAIT);
				schedule_timeout(period);
				enable_timeout_irq();
			}
			unmask_irq(prev_mask);
			while (!wait_done);
		}

	};

}