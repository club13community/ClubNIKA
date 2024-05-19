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
#include "./uart_ctrl.h"

namespace sim900 {
	template <void (*end)(Result)>
	void end_on_timeout() {
		end(Result::NO_RESPONSE);
	}

	/** Applicable to commands with optional response with information */
	template <const char * (& prefix), void (*parse)(rx_buffer_t &), void (*end)(Result)>
	bool get_info_listener(sim900::rx_buffer_t & rx) {
		if (!is_sent()) {
			return false;
		}
		if (rx.is_message_corrupted()) {
			end(Result::CORRUPTED_RESPONSE);
			return true;
		} else if (rx.starts_with(prefix)) {
			parse(rx);
			start_response_timeout(RESP_TIMEOUT_ms, end_on_timeout<end>);
			return true;
		} else if (rx.equals("OK")) {
			end(Result::OK);
			return true;
		} else if (rx.equals("ERROR") || rx.starts_with("+CME ERROR:")) {
			end(Result::ERROR);
			return true;
		}
		return false; // not a message for this handler
	}

	template <const char * (& prefix), void (*parse)(rx_buffer_t &), void (*end)(Result)>
	void start_get_info(const char * cmd, uint16_t len, uint32_t deadline_ms) {
		begin_command(get_info_listener<prefix, parse, end>);
		send_with_timeout(cmd, len, deadline_ms, end_on_timeout<end>);
	}

	template <void (* end)(Result)>
	bool execute_listener(rx_buffer_t & rx) {
		if (!is_sent()) {
			return false;
		}
		if (rx.is_message_corrupted()) {
			end(Result::CORRUPTED_RESPONSE);
			return true;
		} else if (rx.equals("OK")) {
			end(Result::OK);
			return true;
		} else if (rx.equals("ERROR") || rx.starts_with("+CME ERROR:")) {
			end(Result::ERROR);
			return true;
		}
		return false; // not a message for this handler
	}

	template <void (* end)(Result)>
	void start_execute(const char * cmd, uint16_t len, uint32_t deadline_ms) {
		begin_command(execute_listener<end>);
		send_with_timeout(cmd, len, deadline_ms, end_on_timeout<end>);
	}
}