//
// Created by independent-variable on 5/14/2024.
//
#include "sim900.h"
#include "./execution.h"
#include "./utils.h"
#include "./config.h"

using namespace sim900;

static void (* volatile handler)(uint16_t, Result);
static char * volatile text_buf;
static volatile uint16_t text_len;
static enum {WAIT_INPUT, WAIT_ID, WAIT_OK} state;

static void send_sms_timeout() {

}

static bool send_sms_listener(rx_buffer_t & rx) {
	auto state_now = state;
	if (state_now == WAIT_INPUT) {
		if (rx.is_message_corrupted()) {
			// start cancelling
			return true;
		} else if (rx.equals("> ")) {
			state = WAIT_ID;
			send_with_timeout(text_buf, text_len, SEND_SMS_TIMEOUT_ms, send_sms_timeout);
			return true;
		}
		return false;
	} else if (state_now == WAIT_ID) {

	} else if (state_now == WAIT_OK) {

	}
	return false;
}

void sim900::send_sms(const char * phone, const char * text, void (* callback)(uint16_t id, Result result)) {
	handler = callback;
	// put command
	char * tail = copy("AT+CMGS=\"", tx_buffer);
	tail = copy(phone, tail);
	tail = copy("\"\r", tail);
	uint16_t cmd_len = tail - tx_buffer;
	// put text
	text_buf = tail;
	const char * text_end = tx_buffer + (TX_BUFFER_LENGTH - 2); // pointer to ending '\0' + 0x1A
	const char * text_char = text;
	while (*text_char != '\0' && tail < text_end) {
		*tail++ = *text_char++;
	}
	*tail++ = '\0';
	*tail++ = 0x1AU; // Ctrl+Z - req. by spec.
	text_len = text_buf - tail;
	// send command
	state = WAIT_INPUT;
	begin_command(send_sms_listener);
	send_with_timeout(tx_buffer, cmd_len, RESP_TIMEOUT_ms, send_sms_timeout);
}