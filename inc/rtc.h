//
// Created by independent-variable on 5/19/2024.
//

#pragma once
#include <stdint.h>

namespace rtc {
	enum class Month : uint8_t {JAN = 1, FEB, MAR, APR, MAY, JUN, JUL, AUG, SEP, OCT, NOV, DEC};

	struct Time {
		uint8_t hour;
		uint8_t minute;
		uint8_t second;
	};

	struct Date {
		uint16_t year;
		Month month;
		uint8_t day;
	};

	/** Timestamp which holds local time and date. */
	struct DateTime {
		Date date;
		Time time;
	};

	/** Timestamp which holds amount of time since system started. */
	struct RunTime {
		uint8_t seconds;
		uint8_t minutes;
		uint8_t hours;
		uint16_t days;
	};

	/** Helper which enables polymorphic return without dynamic memory allocation. */
	class Timestamp {
	private:
		union {
			RunTime run_time;
			DateTime date_time;
		} value;
		bool runtime;
	public:
		Timestamp(RunTime time) : value({.run_time = time}), runtime(true) {}
		Timestamp(DateTime time) : value({.date_time = time}), runtime(false) {}

		inline bool is_runtime() {
			return runtime;
		}

		inline RunTime & as_runtime() {
			return value.run_time;
		}

		inline DateTime & as_datetime() {
			return value.date_time;
		}
	};

	void start();
	/** System does not have real RTC IC. Use this to init/update date and time entered by user
	 * or retrieved from other source. */
	void set(DateTime & timestamp);
	/** @returns DateTime if it was set otherwise RunTime. */
	Timestamp now();
}