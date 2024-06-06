//
// Created by independent-variable on 5/5/2024.
//
#include "logging.h"
#include "./Buffer.h"
#include <cstring>
#include "FreeRTOS.h"
#include "task.h"
#include "periph_allocation.h"
#include "./config.h"
#include "./sd_recorder.h"
#include "rtc.h"

namespace rec {
	typedef Buffer<9, MAX_MESSAGE_LENGTH + 1> LogBuffer;
}

static rec::LogBuffer buffer;
static char message[MAX_MESSAGE_LENGTH + 2]; // message + "\n\0"

#define STACK_SIZE	256U
static StackType_t stack[STACK_SIZE];
static StaticTask_t task_ctrl;

static void record_logs(void * args) {
	// todo: log bulk of messages, not each message
	rec::init_card_recorder();
	while (true) {
		auto slot = buffer.start_read();
		uint16_t len = slot.copy_to(message);
		message[len++] = '\n';
		message[len] = '\0';

		rec::write_to_card(message, len);

		buffer.end_read();
	}
}

void rec::start() {
	xTaskCreateStatic(record_logs, "logger", STACK_SIZE, nullptr, TASK_NORMAL_PRIORITY, stack, &task_ctrl);
}

rec::s::s(int32_t val) {
	char * dig_start = text;
	if (val < 0) {
		text[0] = '-';
		dig_start = text + 1;
		val = -val;
	}
	char * dig_end = dig_start;

	// put digits in reverse order
	do {
		char dig = (val % 10) | 0x30;
		*dig_end++ = dig;
		val /= 10;
	} while (val != 0);
	*dig_end-- = '\0';

	// reverse order of digits
	while (dig_start < dig_end) {
		char dig = *dig_start;
		*dig_start++ = *dig_end;
		*dig_end-- = dig;
	}
}

/** Check's if buff points to '{n}'. If yes - returns n, otherwise - -1 */
static inline int8_t get_arg_index(const char * buff) {
	if (buff[0] == '{' && buff[2] == '}') {
		char n = buff[1];
		return n >= '0' && n <= '9' ? n & 0x0F : -1;
	} else {
		return -1;
	}
}

static inline uint16_t len_of(std::initializer_list<const char *> & args) {
	uint16_t len = 0;
	for (uint8_t i = 0; i < args.size(); i++) {
		len += strlen(args.begin()[i]);
	}
	return len;
}

static uint16_t len_of(rtc::Timestamp & time);
static uint16_t set(rtc::Timestamp & time, uint16_t index, rec::LogBuffer::Slot & slot);

void rec::log(const char * msg, std::initializer_list<const char *> args) {
	rtc::Timestamp timestamp = rtc::now();
	// message format: "<timestamp>: <msg with args>"
	uint16_t msg_len = len_of(timestamp) + 2U + strlen(msg) + len_of(args);
	LogBuffer::Slot slot = buffer.start_write(msg_len + 1);
	if (slot.is_empty()) {
		return;
	}
	uint16_t dst_ind = set(timestamp, 0, slot);
	dst_ind = slot.set(dst_ind, ": ");
	const char * src = msg;
	while (*src != '\0') {
		int8_t arg_index = get_arg_index(src);
		if (arg_index < 0 || arg_index >= args.size()) {
			// usual char or non-existing argument
			slot.set(dst_ind++, *src++);
		} else {
			// start of message argument
			dst_ind = slot.set(dst_ind, args.begin()[arg_index]);
			src += 3;
		}
	}
	slot.set(dst_ind, '\0');
	buffer.end_write(slot);
}

void rec::log(const char * msg) {
	rtc::Timestamp timestamp = rtc::now();
	// message format: "<timestamp>: <msg>"
	uint16_t msg_len = len_of(timestamp) + 2U + strlen(msg);
	LogBuffer::Slot slot = buffer.start_write(msg_len + 1);
	if (slot.is_empty()) {
		return;
	}
	slot.set(0, msg);
	buffer.end_write(slot);
}

static inline uint8_t len_of(uint16_t val) {
	if (val >= 10'000U) {
		return 5U;
	}
	if (val >= 1'000U) {
		return 4U;
	}
	if (val >= 100U) {
		return 3U;
	}
	if (val >= 10U) {
		return 2U;
	}
	return 1U;
}

static uint16_t len_of(rtc::Timestamp & time) {
	if (time.is_runtime()) {
		// return length of a string formatted as "9999d 1h 59m 3s" without leading 0-values
		rtc::RunTime & runtime = time.as_runtime();
		uint8_t len = len_of(runtime.seconds) + 1U;
		bool has_higher;
		if ((has_higher = runtime.days > 0)) {
			len += len_of(runtime.days) + 2U;
		}
		if ((has_higher != runtime.hours > 0)) {
			len += len_of(runtime.hours) + 2U;
		}
		if (runtime.minutes > 0 || has_higher) {
			len += len_of(runtime.minutes) + 2U;
		}
		return len;
	} else {
		// return length of a string formatted as "dd/mm/yy hh:mm:ss"
		return 17U;
	}
}

static uint16_t set(rtc::RunTime & timestamp, uint16_t index, rec::LogBuffer::Slot & slot);
static uint16_t set(rtc::DateTime & timestamp, uint16_t index, rec::LogBuffer::Slot & slot);

static uint16_t set(rtc::Timestamp & time, uint16_t index, rec::LogBuffer::Slot & slot) {
	if (time.is_runtime()) {
		return set(time.as_runtime(), index, slot);
	} else {
		return set(time.as_datetime(), index, slot);
	}
}

/** @param val converted without leading '0's. */
static uint16_t set(uint16_t val, uint16_t index, rec::LogBuffer::Slot & slot);

static uint16_t set(rtc::RunTime & timestamp, uint16_t index, rec::LogBuffer::Slot & slot) {
	bool has_higher;
	if ((has_higher = timestamp.days > 0)) {
		index = set(timestamp.days, index, slot);
		index = slot.set(index, "d ");
		has_higher = true;
	}
	if ((has_higher |= timestamp.hours > 0)) {
		index = set(timestamp.hours, index, slot);
		index = slot.set(index, "h ");
	}
	if (timestamp.minutes > 0 || has_higher) {
		index = set(timestamp.minutes, index, slot);
		index = slot.set(index, "m ");
	}
	set(timestamp.seconds, index, slot);
	slot.set(index++, 's');
	return index;
}

static uint16_t set(rtc::Time & time, uint16_t index, rec::LogBuffer::Slot & slot);
static uint16_t set(rtc::Date & date, uint16_t index, rec::LogBuffer::Slot & slot);

static uint16_t set(rtc::DateTime & timestamp, uint16_t index, rec::LogBuffer::Slot & slot) {
	index = set(timestamp.time, index, slot);
	slot.set(index++, ' ');
	index = set(timestamp.date, index, slot);
	return index;
}

static inline uint16_t set_2dig(uint8_t val, uint16_t index, rec::LogBuffer::Slot & slot) {
	char d2 = char(0x30U | val % 10U);
	char d1 = char(0x30U | val / 10);
	slot.set(index, d1);
	slot.set(index + 1U, d2);
	return index + 2U;
}

static uint16_t set(rtc::Time & time, uint16_t index, rec::LogBuffer::Slot & slot) {
	index = set_2dig(time.hour, index, slot);
	slot.set(index++, ':');
	index = set_2dig(time.minute, index, slot);
	slot.set(index++, ':');
	index = set_2dig(time.second, index, slot);
	return index;
}

static uint16_t set(rtc::Date & date, uint16_t index, rec::LogBuffer::Slot & slot) {
	index = set_2dig(date.day, index, slot);
	slot.set(index++, '/');
	index = set_2dig((uint8_t)date.month, index, slot);
	slot.set(index++, '/');
	index = set_2dig((uint8_t)(date.year % 100U), index, slot);
	return index;
}

static uint16_t set(uint16_t val, uint16_t index, rec::LogBuffer::Slot & slot) {
	char buf[5];
	char * dig = buf;
	do {
		*dig++ = val % 10U | 0x30U;
		val /= 10U;
	} while (val > 0);
	uint16_t end = index + (buf - dig);
	// buf[] contains digits in rev. order, put them to slot starting from tail
	uint16_t head = index;
	do {
		slot.set(head++, *--dig);
	} while (dig != buf);
	return end;
}