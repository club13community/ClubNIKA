//
// Created by independent-variable on 5/15/2024.
// Template method for typical "get info" command. SIM900 responds to them like:
//	+PREFIX: param1, param2, ..
// 	OK
// or
// 	ERROR
// or
//	+CMS ERROR: code
//

#pragma once
#include "sim900.h"
#include "./execution.h"
#include "./config.h"

namespace sim900 {
	enum class GetInfoState {
		WAIT_INFO, WAIT_OK, WAIT_OPTIONAL_OK, DONE
	};

	template <volatile GetInfoState & state, void (*end)(Result)>
	void get_info_timeout() {
		GetInfoState state_now = state;
		if (state_now == GetInfoState::WAIT_INFO || state_now == GetInfoState::WAIT_OK) {
			state = GetInfoState::DONE;
			end(Result::NO_RESPONSE);
		} else if (state_now == GetInfoState::WAIT_OPTIONAL_OK) {
			state = GetInfoState::DONE;
			end(Result::ERROR); // most probably prev. corrupted message was 'ERROR'
		}
	}

	template <const char * (& prefix), volatile GetInfoState & state, void (*parse)(rx_buffer_t &), void (*end)(Result)>
	bool get_info_handler(sim900::rx_buffer_t & rx) {
		GetInfoState state_now = state;
		if (state_now == GetInfoState::WAIT_INFO) {
			if (rx.is_message_corrupted()) {
				// wait "OK" if SIM900 sent requested info, if SIM900 sent "ERROR" - nothing will be received
				state = GetInfoState::WAIT_OPTIONAL_OK;
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
			}
			return false; // not a message for this handler
		} else if (state_now == GetInfoState::WAIT_OK) {
			state = GetInfoState::DONE;
			end(Result::OK);
			return true;
		} else if (state_now == GetInfoState::WAIT_OPTIONAL_OK) {
			state = GetInfoState::DONE;
			end(Result::CORRUPTED_RESPONSE);
			return true;
		}
		return false; // not a message for this handler
	}
}