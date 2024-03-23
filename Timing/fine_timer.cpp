//
// Created by independent-variable on 3/21/2024.
//
#include "timing.h"
#include "stm32f10x.h"
#include "periph_allocation.h"
#include "tim_utils.h"
#include "rcc_utils.h"
#include "nvic_utils.h"

#define TIMER	FINE_TIMER
#define CR1_RUNNING	TIM_CR1_CEN
#define CR1_STOPPED 0U

void timing::config_fine_timer() {
	uint32_t int_clk = utils::get_int_clock_frequency(TIMER);
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

typedef void (* SetterOfCCR)(TIM_TypeDef *, uint16_t);
static void do_nothing() {}

using namespace timing;

template <SetterOfCCR set_CCRx, uint16_t TIM_SR_CCxIF, uint16_t TIM_DIER_CCxIE, uint16_t TIM_EGR_CCxG>
class TimerChannel : public FineTimer {
	friend void timing::handle_fine_timer_interrupt();
private:
	enum Type : bool {SINGLE = false, REPETITIVE = true};
	volatile CallbackPointer callback;
	volatile uint16_t period;
	volatile bool repeat;

	inline void disable() {
		uint32_t primask = __get_PRIMASK();
		__set_PRIMASK(1U);
		TIMER->DIER &= ~TIM_DIER_CCxIE;
		__set_PRIMASK(primask);
	}

	inline void set_next_moment(uint16_t ticks) {
		uint32_t primask = __get_PRIMASK();
		__set_PRIMASK(1U);
		TIMER->CR1 = CR1_STOPPED;
		set_CCRx(TIMER, TIMER->CNT + ticks);
		TIMER->SR = ~TIM_SR_CCxIF;
		TIMER->CR1 = CR1_RUNNING;
		__set_PRIMASK(primask);
	}

	inline void set_next_moment_and_enable(uint16_t ticks) {
		uint32_t primask = __get_PRIMASK();
		__set_PRIMASK(1U);
		TIMER->CR1 = CR1_STOPPED;
		set_CCRx(TIMER, TIMER->CNT + ticks);
		TIMER->SR = ~TIM_SR_CCxIF;
		TIMER->CR1 = CR1_RUNNING;
		TIMER->DIER |= TIM_DIER_CCxIE;
		__set_PRIMASK(primask);
	}

	inline void start(uint16_t ticks, CallbackPointer callback, Type type) {
		this->callback = callback;
		this->period = ticks + 1;
		this->repeat = type;
		set_next_moment_and_enable(ticks + 1);
	}

	inline void handle_interrupt() {
		if (repeat) {
			callback();
			set_next_moment(period);
		} else {
			disable();
			callback();
		}
	}

public:
	/** May be used by callback */
	void stop() override {
		disable();
	}

	void invoke_in_us(uint16_t delayUs, CallbackPointer callback) override {
		start(delayUs, callback, SINGLE);
	}

	void every_us_invoke(uint16_t periodUs, CallbackPointer callback) override {
		start(periodUs, callback, REPETITIVE);
	}

	void wait_us(uint16_t delayUs) override {
		set_next_moment(delayUs);
		while (!(TIMER->SR & TIM_SR_CCxIF));
	}
};

namespace timing {
	static TimerChannel timer_channel1 = TimerChannel<TIM_SetCompare1, TIM_SR_CC1IF, TIM_DIER_CC1IE, TIM_EGR_CC1G>();
	static TimerChannel timer_channel2 = TimerChannel<TIM_SetCompare2, TIM_SR_CC2IF, TIM_DIER_CC2IE, TIM_EGR_CC2G>();
	static TimerChannel timer_channel3 = TimerChannel<TIM_SetCompare3, TIM_SR_CC3IF, TIM_DIER_CC3IE, TIM_EGR_CC3G>();
	static TimerChannel timer_channel4 = TimerChannel<TIM_SetCompare4, TIM_SR_CC4IF, TIM_DIER_CC4IE, TIM_EGR_CC4G>();
}

void timing::handle_fine_timer_interrupt() {
	uint16_t dier = TIMER->DIER;
	// reading SR after DIER prevents false invoking of callback when more priority ISR enables timer
	uint16_t sr = TIMER->SR;
	if (sr & TIM_SR_CC1IF && dier & TIM_DIER_CC1IE) {
		timer_channel1.handle_interrupt();
	}
	if (sr & TIM_SR_CC2IF && dier & TIM_DIER_CC2IE) {
		timer_channel2.handle_interrupt();
	}
	if (sr & TIM_SR_CC3IF && dier & TIM_DIER_CC3IE) {
		timer_channel3.handle_interrupt();
	}
	if (sr & TIM_SR_CC4IF && dier & TIM_DIER_CC4IE) {
		timer_channel4.handle_interrupt();
	}
}

namespace timing {
	FineTimer & fine_timer1 = timer_channel1;
	FineTimer & fine_timer2 = timer_channel2;
	FineTimer & fine_timer3 = timer_channel3;
	FineTimer & fine_timer4 = timer_channel4;
}