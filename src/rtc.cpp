//
// Created by independent-variable on 5/19/2024.
//
#include "rtc.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "logging.h"

static rtc::RunTime run_time = {0};
static rtc::DateTime date_time;
static uint8_t dst; // current "daylight saving time shift" in hours
static bool has_date_time = false;

static void tick(TimerHandle_t tmr);

void rtc::start() {
	static StaticTimer_t tmr_buff;
	TimerHandle_t tmr = xTimerCreateStatic("rtc", pdMS_TO_TICKS(1000U), pdTRUE, (void *)0, tick, &tmr_buff);
	xTimerStart(tmr, portMAX_DELAY);
}

void rtc::set(rtc::DateTime & timestamp, uint8_t dst_shift) {
	rec::log("Received local time");
	portENTER_CRITICAL();
	date_time = timestamp;
	dst = dst_shift;
	has_date_time = true;
	portEXIT_CRITICAL();
}

void rtc::change_dst(uint8_t dst_shift) {
	portENTER_CRITICAL();
	if (has_date_time) {
		add_hours(dst_shift - dst, date_time);
		dst = dst_shift;
	}
	portEXIT_CRITICAL();
}

rtc::Timestamp rtc::now() {
	Timestamp tm;
	portENTER_CRITICAL();
	if (has_date_time) {
		tm = {date_time};
	} else {
		tm = {run_time};
	}
	portEXIT_CRITICAL();
	return tm;
}

static void inc_run_time();

static void tick(TimerHandle_t tmr) {
	portENTER_CRITICAL();
	inc_run_time();
	if (has_date_time) {
		rtc::add_second(date_time);
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

bool rtc::add_second(Time & time) {
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

int8_t rtc::add_hours(int8_t hours, Time & time) {
	int8_t hr = time.hour + hours;
	if (hr < 0) {
		time.hour = hr + 24;
		return -1;
	} else if (hr > 23) {
		time.hour = hr - 24;
		return 1;
	} else {
		time.hour = hr;
		return 0;
	}
}

uint8_t rtc::days_in_month(Month month, uint16_t year) {
	if (month == Month::APR || month == Month::JUN || month == Month::SEP || month == Month::NOV) {
		return 30U;
	} else if (month == Month::FEB) {
		return year & 0x03U ? 28U : 29U;
	} else {
		return 31U;
	}
}

static inline void inc_month(rtc::Month & month, uint16_t & year) {
	uint8_t nxt = (uint8_t)month + 1U;
	if (nxt > 12U) {
		nxt = 1U;
		year += 1U;
	}
	month = rtc::Month(nxt);
}

static inline void dec_month(rtc::Month & month, uint16_t & year) {
	uint8_t prev = (uint8_t)month - 1;
	if (prev == 0) {
		prev = 12U;
		year -= 1U;
	}
	month = rtc::Month(prev);
}

void rtc::add_days(int8_t days, Date & date) {
	int16_t dy = date.day + days;
	do {
		uint8_t max_dys;
		if (dy < 1) {
			dec_month(date.month, date.year);
			dy += days_in_month(date.month, date.year);
		} else if (dy > (max_dys = days_in_month(date.month, date.year))) {
			dy -= max_dys;
			inc_month(date.month, date.year);
		} else {
			break;
		}
	} while (true);
	date.day = dy;
}

void rtc::add_second(DateTime & timestamp) {
	if (add_second(timestamp.time)) {
		add_days(1U, timestamp.date);
	}
}

void rtc::add_hours(int8_t hours, DateTime & timestamp) {
	int8_t inc_days = add_hours(hours, date_time.time);
	add_days(inc_days, timestamp.date);
}