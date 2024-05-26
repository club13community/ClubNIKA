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

static bool call_listener(rx_buffer_t & rx) {
	if (!is_sent()) {
		return false;
	}
	if (rx.is_message_corrupted()) {
		return false;
	} else if (rx.equals("OK")) {
		call_done(Result::OK);
		return true;
	} else if (rx.equals("ERROR") || rx.equals("NO DIALTONE")
			// haven't seen this, but may be according to spec.
			|| rx.equals("NO CARRIER") || rx.equals("BUSY") || rx.equals("NO ANSWER") || rx.starts_with("+CME ERROR:")) {
		call_done(Result::ERROR);
		return true;
	}
	return false; // not a message for this handler
}

void sim900::call(const char * phone, void (* callback)(Result result)) {
	call_callback = callback;

	// prepare command
	char * tail = copy("ATD", tx_buffer);
	tail = copy(phone, tail); // buffer is always big enough
	*tail++ = ';';
	*tail++ = '\r';
	uint16_t len = tail - tx_buffer;

	begin_command(call_listener);
	send_with_timeout(tx_buffer, len, RESP_TIMEOUT_ms, end_on_timeout<call_done>);
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

/** @param number should have at least MAX_PHONE_LENGTH + 6 items. */
static void parse_call_info(rx_buffer_t & rx, volatile uint8_t & index, volatile CallState & state,
							volatile CallDirection & direction, char * number) {
	// length of phone number + opt. "+country code" + quotes + '\0'
	// MAX_PHONE_LENGTH takes into account last '0' from country code
	constexpr uint8_t num_buffer_len = MAX_PHONE_LENGTH + (1 + 2) + 2 + 1;
	char * const param = number; // use buffer for other params
	// parse index
	rx.get_param(0, param, 1);
	index = 0x0FU & param[0];
	// parse direction
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
	uint16_t len = rx.get_param(5, param, num_buffer_len - 1);
	copy(param, param[1] == '+' ? 4 : 1, len - 1, number);
}

static void (* volatile call_info_data_callback)(uint8_t, CallState, CallDirection, char *);
static void (* volatile call_info_result_callback)(Result);

static void call_info_done(Result res) {
	end_command();
	call_info_result_callback(res);
}

static void parse_call_info(rx_buffer_t & rx) {
	uint8_t index;
	CallState state;
	CallDirection direction;
	parse_call_info(rx, index, state, direction, tx_buffer);
	call_info_data_callback(index, state, direction, tx_buffer);
}

void sim900::get_call_info(void (* data_callback)(uint8_t index, CallState state, CallDirection direction, char * number),
						   void (* result_callback)(Result result)) {
	constexpr const char * cmd = "AT+CLCC\r";
	static const char * CLCC = "+CLCC:";
	call_info_data_callback = data_callback;
	call_info_result_callback = result_callback;

	start_get_info<CLCC, parse_call_info, call_info_done>(cmd, length(cmd), RESP_TIMEOUT_ms);
}

void sim900::wait_call_state(
		bool (* predicate)(uint8_t index, CallState state, CallDirection direction, const char * number),
		uint32_t deadline_ms, void (* callback)(bool state_reached)) {
	static bool (* volatile desired_state)(uint8_t, CallState, CallDirection, const char * number);
	static void (* volatile end_wait)(bool);

	desired_state = predicate;
	end_wait = callback;

	static constexpr auto listener = [](rx_buffer_t & rx) {
		if (rx.is_message_corrupted()) {
			return false;
		}
		if (!rx.starts_with("+CLCC:")) {
			return false;
		}
		uint8_t index;
		CallState state;
		CallDirection direction;
		char number[MAX_PHONE_LENGTH + 6];
		parse_call_info(rx, index, state, direction, number);
		if (desired_state(index, state, direction, number)) {
			end_command();
			end_wait(true);
			return true;
		} else {
			return false;
		}
	};

	static constexpr auto timeout = []() {
		end_command();
		end_wait(false);
	};

	begin_command(listener);
	start_response_timeout(deadline_ms, timeout);
}

bool sim900::call_state_listener(rx_buffer_t & rx) {
	if (!rx.starts_with("+CLCC:")) {
		return false;
	}

	uint8_t index;
	CallDirection direction;
	CallState state;
	char number[MAX_PHONE_LENGTH + 6];
	parse_call_info(rx, index, state, direction, number);

	on_call_update(index, state, direction, number);
	return true;
}

static inline bool is_call_end(rx_buffer_t & rx, CallEnd & end_type) {
	if (rx.is_message_corrupted()) {
		return false;
	}
	if (rx.equals("NO CARRIER")) {
		end_type = CallEnd::NORMAL;
		return true;
	}
	if (rx.equals("BUSY")) {
		end_type = CallEnd::BUSY;
		return true;
	}
	if (rx.equals("NO ANSWER")) {
		end_type = CallEnd::NO_ANSWER;
		return true;
	}
	if (rx.equals("NO DIALTONE")) {
		end_type = CallEnd::NETWORK_ERROR;
		return true;
	}
	return false;
}

void sim900::wait_call_end(uint32_t deadline_ms, void (* callback)(bool ended, CallEnd end_type)) {
	static void (* volatile end_wait)(bool, CallEnd);

	static constexpr auto listener = [](rx_buffer_t & rx) {
		CallEnd end_type;
		if (is_call_end(rx, end_type)) {
			end_command();
			end_wait(true, end_type);
			return true;
		} else {
			return false;
		}
	};

	static constexpr auto timeout = []() {
		end_command();
		end_wait(false, CallEnd::NORMAL);
	};

	end_wait = callback;
	begin_command(listener);
	start_response_timeout(deadline_ms, timeout);
}

bool sim900::call_end_listener(rx_buffer_t & rx) {
	CallEnd end_type;
	if (is_call_end(rx, end_type)) {
		on_call_end(end_type);
		return true;
	} else {
		return false;
	}
}
