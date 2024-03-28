//
// Created by independent-variable on 3/21/2024.
//
#include "timing.h"
#include "stm32f10x.h"
#include "periph_allocation.h"
#include "tim_utils.h"
#include "rcc_utils.h"
#include "nvic_utils.h"
#include "timer_channel.h"

#define TIMER	FINE_TIMER
#define CR1_RUNNING	TIM_CR1_CEN
#define CR1_STOPPED 0U

void timing::config_fine_timer() {
	uint32_t int_clk = get_int_clock_frequency(TIMER);
	uint32_t ratio = int_clk / 1000000U;
	if (ratio == 0) {
		throw std::exception();
	}
	uint32_t reminder = int_clk % 1000000U;
	if (reminder != 0) {
		throw std::exception();
	}

	enable_periph_clock(TIMER);
	TIMER->CR1 = CR1_STOPPED;
	TIMER->CR2 = 0U;
	TIMER->SMCR = 0U;
	TIMER->ARR = UINT16_MAX;
	TIMER->PSC = ratio - 1;

	TIM_OCInitTypeDef oc_conf = {0};
	oc_conf.TIM_OCMode = TIM_OCMode_Timing;
	TIM_OC1Init(TIMER, &oc_conf);
	TIM_OC2Init(TIMER, &oc_conf);
	TIM_OC3Init(TIMER, &oc_conf);
	TIM_OC4Init(TIMER, &oc_conf);

	IRQn_Type irqn = get_IRQn(TIMER);
	NVIC_SetPriority(irqn, FINE_TIMER_IRQ_PRIORITY);
	NVIC_EnableIRQ(irqn);

	TIMER->CR1 = CR1_RUNNING;
}

namespace timing {

	template <SetterOfCCR set_CCRx, uint16_t TIM_SR_CCxIF, uint16_t TIM_DIER_CCxIE>
	class FineTimerChannel : public FineTimer, public TimerChannel<set_CCRx, TIM_SR_CCxIF, TIM_DIER_CCxIE>{
		friend void timing::handle_fine_timer_interrupt();
	public:
		FineTimerChannel(TIM_TypeDef * tim) : TimerChannel<set_CCRx, TIM_SR_CCxIF, TIM_DIER_CCxIE>(tim) {}

		void stop() override {
			TimerChannel<set_CCRx, TIM_SR_CCxIF, TIM_DIER_CCxIE>::stop();
		}

		void invoke_in_us(uint16_t delayUs, Callback callback) override {
			TimerChannel<set_CCRx, TIM_SR_CCxIF, TIM_DIER_CCxIE>::invoke_in_ticks(delayUs, callback);
		}

		void every_us_invoke(uint16_t periodUs, Callback callback) override {
			TimerChannel<set_CCRx, TIM_SR_CCxIF, TIM_DIER_CCxIE>::every_ticks_invoke(periodUs, callback);
		}

		void wait_us(uint16_t delayUs) override {
			TimerChannel<set_CCRx, TIM_SR_CCxIF, TIM_DIER_CCxIE>::wait_ticks(delayUs);
		}
	};

	static FineTimerChannel timer_channel1 = FineTimerChannel<TIM_SetCompare1, TIM_SR_CC1IF, TIM_DIER_CC1IE>(TIMER);
	static FineTimerChannel timer_channel2 = FineTimerChannel<TIM_SetCompare2, TIM_SR_CC2IF, TIM_DIER_CC2IE>(TIMER);
	static FineTimerChannel timer_channel3 = FineTimerChannel<TIM_SetCompare3, TIM_SR_CC3IF, TIM_DIER_CC3IE>(TIMER);
	static FineTimerChannel timer_channel4 = FineTimerChannel<TIM_SetCompare4, TIM_SR_CC4IF, TIM_DIER_CC4IE>(TIMER);
}

void timing::handle_fine_timer_interrupt() {
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
	FineTimer & fine_timer1 = timer_channel1;
	FineTimer & fine_timer2 = timer_channel2;
	FineTimer & fine_timer3 = timer_channel3;
	FineTimer & fine_timer4 = timer_channel4;
}