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

namespace rec {
	typedef Buffer<9, MAX_MESSAGE_LENGTH + 1> LogBuffer;
}

static rec::LogBuffer buffer;
static char message[MAX_MESSAGE_LENGTH + 2]; // message + "\n\0"

#define STACK_SIZE	256U
static StackType_t stack[STACK_SIZE];
static StaticTask_t task_ctrl;

static void record_logs(void * args) {
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

static inline uint16_t args_len(std::initializer_list<const char *> & args) {
	uint16_t len = 0;
	for (uint8_t i = 0; i < args.size(); i++) {
		len += strlen(args.begin()[i]);
	}
	return len;
}

void rec::log(const char * msg, std::initializer_list<const char *> args) {
	uint16_t msg_len = strlen(msg) + args_len(args);
	LogBuffer::Slot slot = buffer.start_write(msg_len + 1);
	if (slot.is_empty()) {
		return;
	}
	const char * src = msg;
	uint16_t dst_ind = 0;
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
	uint16_t msg_len = strlen(msg);
	LogBuffer::Slot slot = buffer.start_write(msg_len + 1);
	if (slot.is_empty()) {
		return;
	}
	slot.set(0, msg);
	buffer.end_write(slot);
}

