//
// Created by independent-variable on 5/14/2024.
//
#include "sim900.h"
#include "sim900_callbacks.h"
#include "./call_ctrl.h"
#include "./execution.h"
#include "./ctrl_templates.h"
#include "./utils.h"
#include "settings.h"

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

/** @param number should have at least MAX_PHONE_LENGTH + 6 items. */
static void parse_call_info(rx_buffer_t & rx,
							volatile CallState & state, volatile CallDirection & direction, char * number) {
	// length of phone number + opt. "+country code" + quotes + '\0'
	// MAX_PHONE_LENGTH takes into account last '0' from country code
	constexpr uint8_t max_param_len = MAX_PHONE_LENGTH + (1 + 2) + 2 + 1;
	// parse direction
	char * const param = number; // use buffer for other params
	rx.get_param(1, param, 1);
	direction = param[0] == '0' ? CallDirection::OUTGOING : CallDirection::INCOMING;
	// parse state
	rx.get_param(2, param, 1);
	if (param[0] == '0') {
		state = CallState::SPEAKING;
	} else if (param[0] == '1') {
		state = CallState::HELD;
	} else if (param[0] == '2' || param[0] == '3' || param[0] == '4') {
		// 2, 3 - for outgoing call, before and during ringing; 4 - incoming ringing
		state = CallState::RINGING;
	} else if (param[0] == '5'){
		state = CallState::WAITING;
	} else { // '6'
		state = CallState::ENDED;
	}
	// parse phone number(is quoted)
	uint16_t len = rx.get_param(5, param, max_param_len - 1);
	copy(param, param[1] == '+' ? 4 : 1, len - 1, number);
}

static void call_info_done(Result res) {
	end_command();
	call_info_callback(call_info_state, call_info_direction, tx_buffer, res);
}

static void parse_call_info(rx_buffer_t & rx) {
	parse_call_info(rx, call_info_state, call_info_direction, tx_buffer);
}

void sim900::get_call_info(void (* callback)(CallState state,  CallDirection direction, char * number, Result result)) {
	constexpr const char * cmd = "AT+CLCC\r";
	static const char * CLCC = "+CLCC:";
	call_info_callback = callback;
	call_info_state = CallState::ENDED;
	call_info_direction = CallDirection::INCOMING;

	start_get_info<CLCC, parse_call_info, call_info_done>(cmd, length(cmd), RESP_TIMEOUT_ms);
}


bool sim900::call_state_listener(rx_buffer_t & rx) {
	if (!rx.starts_with("+CLCC:")) {
		return false;
	}

	CallDirection direction;
	CallState state;
	char number[MAX_PHONE_LENGTH + 6];
	parse_call_info(rx, state, direction, number);

	on_call_update(state, direction, number);
	return true;
}

bool sim900::call_end_listener(rx_buffer_t & rx) {
	if (rx.equals("NO CARRIER")) {
		on_call_end(CallEnd::NORMAL);
		return true;
	}
	if (rx.equals("BUSY")) {
		on_call_end(CallEnd::BUSY);
		return true;
	}
	if (rx.equals("NO ANSWER")) {
		on_call_end(CallEnd::NO_ANSWER);
		return true;
	}
	return false;
}
