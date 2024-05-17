//
// Created by independent-variable on 5/14/2024.
//
#include "sim900.h"
#include "./execution.h"
#include "./ctrl_templates.h"
#include "./utils.h"

using namespace sim900;

static void (* volatile call_callback)(Result);

static void call_done(Result res) {
	end_command();
	call_callback(res);
}

void sim900::call(const char * phone, void (* callback)(Result result)) {
	static volatile bool active;
	call_callback = callback;

	// prepare command
	char * tail = copy("ATD", tx_buffer);
	tail = copy(phone, tail); // buffer is always big enough
	*tail++ = ';';
	*tail++ = '\r';
	uint16_t len = tail - tx_buffer;

	start_execute<active, call_done>(tx_buffer, len, RESP_TIMEOUT_ms);
}

static void (* volatile accept_callback)(Result);

static void accept_done(Result res) {
	end_command();
	accept_callback(res);
}

void sim900::accept_call(void (* callback)(Result result)) {
	constexpr const char * cmd = "ATA\r";
	static volatile bool active;

	accept_callback = callback;
	start_execute<active, accept_done>(cmd, length(cmd), RESP_TIMEOUT_ms);
}

static void (* volatile end_callback)(Result);

static void end_done(Result res) {
	end_command();
	end_callback(res);
}

void sim900::end_call(void (* callback)(Result result)) {
	constexpr const char * cmd = "ATH\r";
	static volatile bool active;

	end_callback = callback;
	start_execute<active, end_done>(cmd, length(cmd), RESP_TIMEOUT_ms);
}

namespace sim900 {
	enum class CallInfoPhase {WAIT_INFO, WAIT_OK, WAIT_OPTIONAL_END, DONE};
}

static void (* volatile call_info_callback)(CallState, char *, Result);
static volatile CallState call_info_state;
static volatile CallInfoPhase call_info_phase;

static void call_info_timeout() {
	auto phase_now = call_info_phase;
	if (phase_now == CallInfoPhase::WAIT_INFO || phase_now == CallInfoPhase::WAIT_OK) {
		call_info_phase = CallInfoPhase::DONE;
		end_command();
		call_info_callback(CallState::ENDED, nullptr, Result::NO_RESPONSE);
	} else if (phase_now == CallInfoPhase::WAIT_OPTIONAL_END) {
		call_info_phase = CallInfoPhase::DONE;
		end_command();
		call_info_callback(CallState::ENDED, nullptr, Result::CORRUPTED_RESPONSE);
	}
}

static bool call_info_listener(rx_buffer_t & rx) {
	auto phase_now = call_info_phase;
	if (phase_now == CallInfoPhase::WAIT_INFO) {
		if (!is_sent()) {
			return false;
		} else if (rx.is_message_corrupted()) {
			// assume that it was requested info
			call_info_phase = CallInfoPhase::WAIT_OPTIONAL_END;
			start_response_timeout(RESP_TIMEOUT_ms, call_info_timeout);
			return true;
		} else if (rx.starts_with("+CLCC:")) {
			// parse state
			char param[2];
			rx.get_param(2, param, 1);
			if (param[0] == '0') {
				call_info_state = CallState::DIALED;
			} else if (param[0] == '6') {
				// did not observe during experiments
				call_info_state = CallState::ENDED;
			} else {
				// observed only 2, 3, 4
				call_info_state = CallState::RINGING;
			}
			// parse phone number(is quoted), tx_buffer is already free
			uint16_t len = rx.get_param(5, tx_buffer, TX_BUFFER_LENGTH);
			copy(tx_buffer, 1, len - 1, tx_buffer);
			call_info_phase = CallInfoPhase::WAIT_OK;
			start_response_timeout(RESP_TIMEOUT_ms, call_info_timeout);
			return true;
		} else if (rx.equals("OK")) {
			// all calls are ended
			call_info_phase = CallInfoPhase::DONE;
			end_command();
			call_info_callback(CallState::ENDED, nullptr, Result::OK);
			return true;
		} else if (rx.equals("ERROR") || rx.starts_with("+CME ERROR:")) {
			// did not observe during experiments
			call_info_phase = CallInfoPhase::DONE;
			end_command();
			call_info_callback(CallState::ENDED, nullptr, Result::ERROR);
			return true;
		} else {
			return false;
		}
	} else if (phase_now == CallInfoPhase::WAIT_OK) {
		call_info_phase = CallInfoPhase::DONE;
		end_command();
		call_info_callback(call_info_state, tx_buffer, Result::OK);
		return true;
	} else if (phase_now == CallInfoPhase::WAIT_OPTIONAL_END) {
		// info about call was not received
		if (rx.is_message_corrupted() || rx.equals("OK")
				|| rx.equals("ERROR") || rx.starts_with("+CME ERROR:")) { // did not observe during experiments
			call_info_phase = CallInfoPhase::DONE;
			end_command();
			call_info_callback(CallState::ENDED, nullptr, Result::CORRUPTED_RESPONSE);
			return true;
		} else {
			return false;
		}
	}
	return false;
}

void sim900::get_call_info(void (* callback)(CallState state, char * number, Result result)) {
	constexpr const char * cmd = "AT+CLCC\r";
	call_info_callback = callback;
	call_info_phase = CallInfoPhase::WAIT_INFO;
	begin_command(call_info_listener);
	send_with_timeout(cmd, length(cmd), RESP_TIMEOUT_ms, call_info_timeout);
}

