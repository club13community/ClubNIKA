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

static void (* volatile result_handler)(Result); // keeps callback, which requires only Result

static bool call_listener(rx_buffer_t & rx) {
	if (!is_sent()) {
		return false;
	}
	if (rx.is_message_corrupted()) {
		end_with<result_handler>(Result::CORRUPTED_RESPONSE);
		return true;
	} else if (rx.equals("OK")) {
		end_with<result_handler>(Result::OK);
		return true;
	} else if (rx.equals("ERROR") || rx.equals("NO DIALTONE")
			// haven't seen this, but may be according to spec.
			|| rx.equals("NO CARRIER") || rx.equals("BUSY") || rx.equals("NO ANSWER") || rx.starts_with("+CME ERROR:")) {
		end_with<result_handler>(Result::ERROR);
		return true;
	}
	return false; // not a message for this handler
}

void sim900::call(const char * phone, void (* callback)(Result result)) {
	result_handler = callback;

	// prepare command
	char * tail = copy("ATD", tx_buffer);
	tail = copy(phone, tail); // buffer is always big enough
	*tail++ = ';';
	*tail++ = '\r';
	uint16_t len = tail - tx_buffer;

	begin_command(call_listener);
	send_with_timeout(tx_buffer, len, RESP_TIMEOUT_ms, end_on_timeout<end_with<result_handler>>);
}

void sim900::accept_call(void (* callback)(Result result)) {
	constexpr const char * cmd = "ATA\r";

	result_handler = callback;
	start_execute<end_with<result_handler>>(cmd, length(cmd), RESP_TIMEOUT_ms);
}

void sim900::end_call(void (* callback)(Result result)) {
	constexpr const char * cmd = "ATH\r";

	result_handler = callback;
	start_execute<end_with<result_handler>>(cmd, length(cmd), RESP_TIMEOUT_ms);
}

void sim900::enable_dtmf(uint16_t debounce_ms, void (* callback)(Result result)) {
	result_handler = callback;
	// prepare command
	char * tail = copy("AT+DDET=1,", tx_buffer); // command + enable arg.
	tail = to_string(debounce_ms, tail); // debounce arg.
	tail = copy(",0\r", tail); // "report only key" arg.
	uint16_t len = tail - tx_buffer;

	start_execute<end_with<result_handler>>(tx_buffer, len, RESP_TIMEOUT_ms);
}

void sim900::disable_dtmf(void (* callback)(Result result)) {
	constexpr const char * cmd = "AT+DDET=0\r";

	result_handler = callback;
	start_execute<end_with<result_handler>>(cmd, length(cmd), RESP_TIMEOUT_ms);
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
	result_handler = result_callback;

	start_get_info<CLCC, parse_call_info, end_with<result_handler>>(cmd, length(cmd), RESP_TIMEOUT_ms);
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
	if (rx.is_message_corrupted()) {
		return false;
	}
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

bool sim900::ring_listener(rx_buffer_t & rx) {
	if (rx.is_message_corrupted()) {
		return false;
	}
	if (!rx.equals("RING")) {
		return false;
	}
	on_ring();
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

	static constexpr auto on_timeout = []() {
		end_command();
		end_wait(false, CallEnd::NORMAL);
	};

	end_wait = callback;
	begin_command(listener);
	start_response_timeout(deadline_ms, on_timeout);
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
