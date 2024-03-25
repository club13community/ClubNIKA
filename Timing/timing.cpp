//
// Created by independent-variable on 3/15/2024.
//
#include "periph_allocation.h"
#include "timing.h"
#include "rcc_utils.h"
#include "tim_utils.h"
#include "nvic_utils.h"

#define TIMER		TIMING_TIMER
#define TIMER_IRQn_PRIORITY		3

/** Timer resolution is 250us(4kHz) or a little more if "internal clock"/4KHz has remainder */
void timing::configPeripherals() {
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
	NVIC_SetPriority(irqn, TIMER_IRQn_PRIORITY);
	NVIC_EnableIRQ(irqn);

	TIM_Cmd(TIMER, ENABLE);
}

typedef void (* SetterOfCCR)(TIM_TypeDef *, uint16_t);
static void do_nothing() {}

using namespace timing;

template <SetterOfCCR set_CCRx, uint16_t TIM_SR_CCxIF, uint16_t TIM_DIER_CCxIE, uint16_t TIM_EGR_CCxG>
class TimerChannel : public Timer {
	friend void timing::handleIrq();
private:
	enum Type : bool {SINGLE = false, REPETITIVE = true};
	volatile CallbackPointer callback;
	volatile uint16_t period;
	volatile bool repeat;

	inline void disable() {
		uint16_t dier;
		do {
			dier = __LDREXH((uint16_t *)&TIMER->DIER);
			dier &= ~TIM_DIER_CCxIE;
		} while (__STREXH(dier, (uint16_t *)&TIMER->DIER));
	}

	inline void enable_channel() {
		uint16_t dier;
		do {
			dier = __LDREXH((uint16_t *)&TIMER->DIER);
			dier |= TIM_DIER_CCxIE;
		} while (__STREXH(dier, (uint16_t *)&TIMER->DIER));
	}

	inline void set_next_moment(uint16_t ticks) {
		uint16_t start = TIMER->CNT;
		set_CCRx(TIMER, start + ticks);
		TIMER->SR = ~TIM_SR_CCxIF;
		uint16_t end = TIMER->CNT;
		if (end - start >= ticks) {
			TIMER->EGR = TIM_EGR_CCxG;
		}
	}

	inline void start(uint16_t ticks, CallbackPointer callback, Type type) {
		this->callback = callback;
		this->period = ticks + 1;
		this->repeat = type;
		set_next_moment(ticks + 1);
		enable_channel();
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

	void invoke_in_ms(uint16_t delayMs, CallbackPointer callback) override {
		start(delayMs<<2, callback, SINGLE);
	}

	void invoke_in_ticks(uint16_t delayMsDiv4, CallbackPointer callback) override {
		start(delayMsDiv4, callback, SINGLE);
	}

	void every_ms_invoke(uint16_t periodMs, CallbackPointer callback) override {
		start(periodMs<<2, callback, REPETITIVE);
	}

	void every_ticks_invoke(uint16_t periodMsDiv4, CallbackPointer callback) override {
		start(periodMsDiv4, callback, REPETITIVE);
	}

	void wait_ms(uint16_t delayMs) override {
		start(delayMs<<2, do_nothing, SINGLE);
		while (!(TIMER->SR & TIM_SR_CCxIF));
	}

	void wait_ticks(uint16_t delayMsDiv4) override {
		start(delayMsDiv4, do_nothing, SINGLE);
		while (!(TIMER->SR & TIM_SR_CCxIF));
	}
};

static TimerChannel timer_channel1 = TimerChannel<TIM_SetCompare1, TIM_SR_CC1IF, TIM_DIER_CC1IE, TIM_EGR_CC1G>();
static TimerChannel timer_channel2 = TimerChannel<TIM_SetCompare2, TIM_SR_CC2IF, TIM_DIER_CC2IE, TIM_EGR_CC2G>();
static TimerChannel timer_channel3 = TimerChannel<TIM_SetCompare3, TIM_SR_CC3IF, TIM_DIER_CC3IE, TIM_EGR_CC3G>();
static TimerChannel timer_channel4 = TimerChannel<TIM_SetCompare4, TIM_SR_CC4IF, TIM_DIER_CC4IE, TIM_EGR_CC4G>();

void timing::handleIrq() {
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
	Timer & timer1 = timer_channel1;
	Timer & timer2 = timer_channel2;
	Timer & timer3 = timer_channel3;
	Timer & timer4 = timer_channel4;
}