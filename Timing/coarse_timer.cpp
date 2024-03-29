//
// Created by independent-variable on 3/15/2024.
//
#include "periph_allocation.h"
#include "timing.h"
#include "rcc_utils.h"
#include "tim_utils.h"
#include "nvic_utils.h"
#include "timer_channel.h"

#define TIMER		TIMING_TIMER

/** Timer resolution is 250us(4kHz) or a little more if "internal clock"/4KHz has remainder */
void timing::config_coarse_timer() {
	uint32_t int_clk = get_int_clock_frequency(TIMER);
	uint32_t ratio = ((int_clk<<1) + 4000U)/(4000U<<1); // int_clk/4kHz + 0.5

	enable_periph_clock(TIMER);

	TIM_TimeBaseInitTypeDef tim_conf = {0};
	tim_conf.TIM_Period = UINT16_MAX;
	tim_conf.TIM_CounterMode = TIM_CounterMode_Up;
	tim_conf.TIM_Prescaler = ratio - 1U;
	tim_conf.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIMER, &tim_conf);

	TIM_OCInitTypeDef oc_conf = {0};
	oc_conf.TIM_OCMode = TIM_OCMode_Timing;
	TIM_OC1Init(TIMER, &oc_conf);
	TIM_OC2Init(TIMER, &oc_conf);
	TIM_OC3Init(TIMER, &oc_conf);
	TIM_OC4Init(TIMER, &oc_conf);

	IRQn_Type irqn = get_IRQn(TIMER);
	NVIC_SetPriority(irqn, COARSE_TIMER_IRQ_PRIORITY);
	NVIC_EnableIRQ(irqn);

	TIM_Cmd(TIMER, ENABLE);
}

namespace timing {

	template<SetterOfCCR set_CCRx, uint16_t TIM_SR_CCxIF, uint16_t TIM_DIER_CCxIE>
	class CoarseTimerChannel : public CoarseTimer, public TimerChannel<set_CCRx, TIM_SR_CCxIF, TIM_DIER_CCxIE> {
		friend void timing::handle_coarse_timer_interrupt();
	public:
		CoarseTimerChannel(TIM_TypeDef * tim) : TimerChannel<set_CCRx, TIM_SR_CCxIF, TIM_DIER_CCxIE>(tim) {}

		void stop() override {
			TimerChannel<set_CCRx, TIM_SR_CCxIF, TIM_DIER_CCxIE>::stop();
		}

		void invoke_in_ms(uint16_t delayMs, Callback callback) override {
			TimerChannel<set_CCRx, TIM_SR_CCxIF, TIM_DIER_CCxIE>::invoke_in_ticks(delayMs << 2, callback);
		}

		void invoke_in_ticks(uint16_t delayMsDiv4, Callback callback) override {
			TimerChannel<set_CCRx, TIM_SR_CCxIF, TIM_DIER_CCxIE>::invoke_in_ticks(delayMsDiv4, callback);
		}

		void every_ms_invoke(uint16_t periodMs, Callback callback) override {
			TimerChannel<set_CCRx, TIM_SR_CCxIF, TIM_DIER_CCxIE>::every_ticks_invoke(periodMs << 2, callback);
		}

		void every_ticks_invoke(uint16_t periodMsDiv4, Callback callback) override {
			TimerChannel<set_CCRx, TIM_SR_CCxIF, TIM_DIER_CCxIE>::every_ticks_invoke(periodMsDiv4, callback);
		}

		void wait_ms(uint16_t delayMs) override {
			TimerChannel<set_CCRx, TIM_SR_CCxIF, TIM_DIER_CCxIE>::wait_ticks(delayMs << 2);
		}

		void wait_ticks(uint16_t delayMsDiv4) override {
			TimerChannel<set_CCRx, TIM_SR_CCxIF, TIM_DIER_CCxIE>::wait_ticks(delayMsDiv4);
		}
	};

	static CoarseTimerChannel timer_channel1 = CoarseTimerChannel<TIM_SetCompare1, TIM_SR_CC1IF, TIM_DIER_CC1IE>(TIMER);
	static CoarseTimerChannel timer_channel2 = CoarseTimerChannel<TIM_SetCompare2, TIM_SR_CC2IF, TIM_DIER_CC2IE>(TIMER);
	static CoarseTimerChannel timer_channel3 = CoarseTimerChannel<TIM_SetCompare3, TIM_SR_CC3IF, TIM_DIER_CC3IE>(TIMER);
	static CoarseTimerChannel timer_channel4 = CoarseTimerChannel<TIM_SetCompare4, TIM_SR_CC4IF, TIM_DIER_CC4IE>(TIMER);
}

void timing::handle_coarse_timer_interrupt() {
	uint16_t dier = TIMER->DIER;
	uint16_t sr = TIMER->SR;

	// invoke callbacks
	bool channel1 = sr & TIM_SR_CC1IF && dier & TIM_DIER_CC1IE;
	if (channel1) {
		timer_channel1.invoke_callback();
	}
	bool channel2 = sr & TIM_SR_CC2IF && dier & TIM_DIER_CC2IE;
	if (channel2) {
		timer_channel2.invoke_callback();
	}
	bool channel3 = sr & TIM_SR_CC3IF && dier & TIM_DIER_CC3IE;
	if (channel3) {
		timer_channel3.invoke_callback();
	}
	bool channel4 = sr & TIM_SR_CC4IF && dier & TIM_DIER_CC4IE;
	if (channel4) {
		timer_channel4.invoke_callback();
	}

	// handle timeout
	uint16_t clear_sr = 0xFFFF;
	__disable_irq();
	TIMER->CR1 &= ~TIM_CR1_CEN;
	if (channel1) {
		timer_channel1.handle_timeout();
		clear_sr &= ~TIM_SR_CC1IF;
	}
	if (channel2) {
		timer_channel2.handle_timeout();
		clear_sr &= ~TIM_SR_CC2IF;
	}
	if (channel3) {
		timer_channel3.handle_timeout();
		clear_sr &= ~TIM_SR_CC3IF;
	}
	if (channel4) {
		timer_channel4.handle_timeout();
		clear_sr &= ~TIM_SR_CC4IF;
	}
	TIMER->SR = clear_sr;
	TIMER->CR1 |= TIM_CR1_CEN;
	__enable_irq();
}

namespace timing {
	CoarseTimer & coarse_timer1 = timer_channel1;
	CoarseTimer & coarse_timer2 = timer_channel2;
	CoarseTimer & coarse_timer3 = timer_channel3;
	CoarseTimer & coarse_timer4 = timer_channel4;
}