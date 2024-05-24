//
// Created by independent-variable on 5/14/2024.
//
#include "sim900.h"
#include "./execution.h"
#include "./ctrl_templates.h"
#include "./utils.h"
#include "./uart_ctrl.h"

using namespace sim900;

static void (* volatile call_callback)(Result);

static void call_done(Result res) {
	end_command();
	call_callback(res);
}

void sim900::call(const char * phone, void (* callback)(Result result)) {
	call_callback = callback;

	// prepare command
	char * tail = copy("ATD", tx_buffer);
	tail = copy(phone, tail); // buffer is always big enough
	*tail++ = ';';
	*tail++ = '\r';
	uint16_t len = tail - tx_buffer;

	start_execute<call_done>(tx_buffer, len, RESP_TIMEOUT_ms);
}

static void (* volatile accept_callback)(Result);

static void accept_done(Result res) {
	end_command();
	accept_callback(res);
}

void sim900::accept_call(void (* callback)(Result result)) {
	constexpr const char * cmd = "ATA\r";

	accept_callback = callback;
	start_execute<accept_done>(cmd, length(cmd), RESP_TIMEOUT_ms);
}

static void (* volatile end_callback)(Result);

static void end_done(Result res) {
	end_command();
	end_callback(res);
}

void sim900::end_call(void (* callback)(Result result)) {
	constexpr const char * cmd = "ATH\r";

	end_callback = callback;
	start_execute<end_done>(cmd, length(cmd), RESP_TIMEOUT_ms);
}

static void (* volatile call_info_callback)(CallState, CallDirection, char *, Result);
static volatile CallState call_info_state;
static volatile CallDirection call_info_direction;

static void call_info_done(Result res) {
	end_command();
	call_info_callback(call_info_state, call_info_direction, tx_buffer, res);
}

static void parse_call_info(rx_buffer_t & rx) {
	char param[2];
	// parse direction
	rx.get_param(1, param, 1);
	if (param[0] == '0') {
		call_info_direction = CallDirection::OUTGOING;
	} else {
		call_info_direction = CallDirection::INCOMING;
	}
	// parse state
	rx.get_param(2, param, 1);
	if (param[0] == '0') {
		call_info_state = CallState::SPEAKING;
	} else if (param[0] == '6') {
		call_info_state = CallState::ENDED;
	} else {
		// observed only 2, 3, 4
		call_info_state = CallState::RINGING;
	}
	// parse phone number(is quoted with opt. country code), tx_buffer is already free
	uint16_t len = rx.get_param(5, tx_buffer, TX_BUFFER_LENGTH);
	copy(tx_buffer, tx_buffer[1] == '+' ? 4 : 1, len - 1, tx_buffer);
}

void sim900::get_call_info(void (* callback)(CallState state,  CallDirection direction, char * number, Result result)) {
	constexpr const char * cmd = "AT+CLCC\r";
	static const char * CLCC = "+CLCC:";
	call_info_callback = callback;
	call_info_state = CallState::ENDED;
	call_info_direction = CallDirection::INCOMING;

	start_get_info<CLCC, parse_call_info, call_info_done>(cmd, length(cmd), RESP_TIMEOUT_ms);
}

