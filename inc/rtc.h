//
// Created by independent-variable on 5/19/2024.
//

#pragma once
#include <stdint.h>

namespace rtc {
	enum class Month : uint8_t {JAN = 1, FEB, MAR, APR, MAY, JUN, JUL, AUG, SEP, OCT, NOV, DEC};

	class Time {
	private:
		inline char * add(uint8_t val, char * buf) const {
			char d2 = char(0x30U | val % 10U);
			char d1 = char(0x30U | val / 10);
			*buf++ = d1;
			*buf++ = d2;
			return buf;
		}
	public:
		const uint8_t hour;
		const uint8_t minute;
		const uint8_t second;

		Time(uint8_t hour, uint8_t minute, uint8_t second) : hour(hour), minute(minute), second(second) {}

		/** Format: "hh:mm:ss"
		 * @param buf should contain at least 9 positions
		 * @return pointer to tailing '\0'*/
		inline char * to_string(char * buf) const {
			char * tail = add(hour, buf);
			*tail++ = ':';
			tail = add(minute, tail);
			*tail++ = ':';
			tail = add(second, tail);
			*tail = '\0';
			return tail;
		}
	};

	class Date {
	private:
		inline char * add(uint8_t val, char * buf) const {
			char d2 = char(0x30U | val % 10U);
			char d1 = char(0x30U | val / 10);
			*buf++ = d1;
			*buf++ = d2;
			return buf;
		}
	public:
		const uint16_t year;
		const Month month;
		const uint8_t day;

		Date(uint16_t year, Month month, uint8_t day) : year(year), month(month), day(day) {}

		/** Format dd/mm/yy.
		 * @returns pointer to tailing '\0' */
		inline char * to_string(char * buf) const {
			char * tail = add(day, buf);
			*tail++ = '/';
			tail = add((uint8_t)month, tail);
			*tail++ = '/';
			tail = add(year % 100, tail);
			*tail = '\0';
			return tail;
		}
	};

	class Timestamp {
	public:
		const Date date;
		const Time time;

		Timestamp(Date & date, Time & time) : date(date), time(time) {}

		/** Format "dd/mm/yy hh:mm:ss".
		 * @param buf should contain 18 positions*/
		char * to_string(char * buf) const;
	};
}