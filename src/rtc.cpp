//
// Created by independent-variable on 5/19/2024.
//
#include "rtc.h"
#include "FreeRTOS.h"
#include "timers.h"

static rtc::RunTime run_time = {0};
static rtc::DateTime date_time;
static bool has_date_time = false;

static void tick(TimerHandle_t tmr);

void rtc::start() {
	static StaticTimer_t tmr_buff;
	TimerHandle_t tmr = xTimerCreateStatic("rtc", pdMS_TO_TICKS(1000U), pdTRUE, (void *)0, tick, &tmr_buff);
	xTimerStart(tmr, portMAX_DELAY);
}

void rtc::set(rtc::DateTime & timestamp) {
	portENTER_CRITICAL();
	date_time = timestamp;
	has_date_time = true;
	portEXIT_CRITICAL();
}

rtc::Timestamp rtc::now() {
	if (has_date_time) {
		return {date_time};
	} else {
		return {run_time};
	}
}

static void inc_run_time();

/** @returns true if next day started. */
static bool inc_time();
static void inc_date();

static inline void inc_date_time() {
	if (inc_time()) {
		inc_date();
	}
}

static void tick(TimerHandle_t tmr) {
	portENTER_CRITICAL();
	inc_run_time();
	if (has_date_time) {
		inc_date_time();
	}
	portEXIT_CRITICAL();
}

static void inc_run_time() {
	uint8_t sec = run_time.seconds + 1U;
	if (sec < 60U) {
		run_time.seconds = sec;
		return;
	}
	run_time.seconds = 0;
	uint8_t min = run_time.minutes + 1U;
	if (min < 60U) {
		run_time.minutes = min;
		return;
	}
	run_time.minutes = 0;
	uint8_t hrs = run_time.hours + 1U;
	if (hrs < 24U) {
		run_time.hours = hrs;
		return;
	}
	run_time.hours = 0;
	run_time.days = run_time.days + 1U;
}

static bool inc_time() {
	rtc::Time & time = date_time.time;
	uint8_t sec = time.second + 1U;
	if (sec < 60U) {
		time.second = sec;
		return false;
	}
	time.second = 0;
	uint8_t min = time.minute + 1U;
	if (min < 60U) {
		time.minute = min;
		return false;
	}
	time.minute = 0;
	uint8_t hr = time.hour + 1U;
	if (hr < 24U) {
		time.hour = hr;
		return false;
	}
	time.hour = 0;
	return true;
}

static void inc_date() {
	using namespace rtc;
	Date & date = date_time.date;
	uint8_t days_in_month;
	Month month = date.month;
	if (month == Month::APR || month == Month::JUN || month == Month::SEP || month == Month::NOV) {
		days_in_month = 30U;
	} else if (month == Month::FEB) {
		days_in_month = date.year & 0x03U ? 28U : 29U;
	} else {
		days_in_month = 31U;
	}
	uint8_t dy = date.day + 1U;
	if (dy <= days_in_month) {
		date.day = dy;
		return;
	}
	date.day = 1;
	uint8_t mth = (uint8_t)month + 1;
	if (mth <= 12U) {
		month = Month(mth);
		return;
	}
	month = Month::JAN;
	date.year = date.year + 1U;
}