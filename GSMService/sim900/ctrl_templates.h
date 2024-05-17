//
// Created by independent-variable on 5/15/2024.
// Communication with SIM900 during typical "get info" command:
// AT+SOMETHING
//	+PREFIX: param1, param2, ..
// 	OK
// or
//	AT+SOMETHING
// 	ERROR
// or
//	AT+SOMETHING
//	+CMS ERROR: code
//
// Communication with SIM900 during typical "execute" command:
//	AT+SOMETHING
// 	OK
// or
//	AT+SOMETHING
// 	ERROR
// or
//	AT+SOMETHING
// 	+CMS ERROR: code
//

#pragma once
#include "sim900.h"
#include "./execution.h"
#include "./config.h"

namespace sim900 {
	enum class GetInfoState {
		WAIT_INFO, WAIT_OK, WAIT_OPTIONAL_END, DONE
	};

	template <volatile GetInfoState & state, void (*end)(Result)>
	void get_info_timeout() {
		GetInfoState state_now = state;
		if (state_now == GetInfoState::WAIT_INFO || state_now == GetInfoState::WAIT_OK) {
			state = GetInfoState::DONE;
			end(Result::NO_RESPONSE);
		} else if (state_now == GetInfoState::WAIT_OPTIONAL_END) {
			state = GetInfoState::DONE;
			end(Result::ERROR); // most probably prev. corrupted message was 'ERROR'
		}
	}

	template <const char * (& prefix), volatile GetInfoState & state, void (*parse)(rx_buffer_t &), void (*end)(Result)>
	bool get_info_handler(sim900::rx_buffer_t & rx) {
		GetInfoState state_now = state;
		if (state_now == GetInfoState::WAIT_INFO) {
			if (!is_sent()) {
				return false;
			} else if (rx.is_message_corrupted()) {
				// wait "OK" if SIM900 sent requested info, if SIM900 sent "ERROR" - nothing will be received
				state = GetInfoState::WAIT_OPTIONAL_END;
				start_response_timeout(RESP_TIMEOUT_ms, get_info_timeout<state, end>);
				return true;
			} else if (rx.starts_with(prefix)) {
				parse(rx);
				state = GetInfoState::WAIT_OK;
				start_response_timeout(RESP_TIMEOUT_ms, get_info_timeout<state, end>);
				return true;
			} else if (rx.equals("ERROR") || rx.starts_with("+CME ERROR:")) {
				state = GetInfoState::DONE;
				end(Result::ERROR);
				return true;
			} else {
				return false; // not a message for this handler
			}
		} else if (state_now == GetInfoState::WAIT_OK) {
			state = GetInfoState::DONE;
			end(Result::OK);
			return true;
		} else if (state_now == GetInfoState::WAIT_OPTIONAL_END) {
			// requested info was not received
			if (rx.is_message_corrupted() || rx.equals("OK") || rx.equals("ERROR") || rx.starts_with("+CME ERROR:")) {
				state = GetInfoState::DONE;
				end(Result::CORRUPTED_RESPONSE);
				return true;
			} else {
				return false;
			}
		}
		return false; // not a message for this handler
	}

	template <const char * (& prefix), volatile GetInfoState & state, void (*parse)(rx_buffer_t &), void (*end)(Result)>
	void start_get_info(const char * cmd, uint16_t len, uint32_t deadline_ms) {
		state = GetInfoState::WAIT_INFO;
		begin_command(get_info_handler<prefix, state, parse, end>);
		send_with_timeout(cmd, len, deadline_ms, get_info_timeout<state, end>);
	}

	template <volatile bool & active, void (* end)(Result)>
	void execute_timeout() {
		if (active) {
			active = false;
			end(Result::NO_RESPONSE);
		}
	}

	template <volatile bool & active, void (* end)(Result)>
	bool execute_handler(rx_buffer_t & rx) {
		if (!active) {
			return false;
		}
		if (!is_sent()) {
			return false;
		} else if (rx.is_message_corrupted()) {
			active = false;
			end(Result::CORRUPTED_RESPONSE);
			return true;
		} else if (rx.equals("OK")) {
			active = false;
			end(Result::OK);
			return true;
		} else if (rx.equals("ERROR") || rx.starts_with("+CME ERROR:")) {
			active = false;
			end(Result::ERROR);
			return true;
		}
		return false;
	}

	template <volatile bool & active, void (* end)(Result)>
	void start_execute(const char * cmd, uint16_t len, uint32_t deadline_ms) {
		active = true;
		begin_command(execute_handler<active, end>);
		send_with_timeout(cmd, len, deadline_ms, execute_timeout<active, end>);
	}
}