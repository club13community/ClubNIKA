//
// Created by independent-variable on 5/14/2024.
//
#include <stdlib.h>
#include "sim900.h"
#include "./uart_ctrl.h"
#include "./execution.h"
#include "./utils.h"
#include "./config.h"

using namespace sim900;

#define CTRL_Z	( (char)0x1AU )
#define ESC		( "\27" ) /* 0x1B */

namespace sim900 {
	enum class SendSmsState {WAIT_INPUT, WAIT_ID, WAIT_OK, WAIT_OPTIONAL_END, CANCELING, DONE};
}

static void (* volatile send_handler)(uint16_t, Result);
static char * volatile send_text_buf;
static volatile uint16_t send_text_len;
static volatile SendSmsState send_state;
static volatile uint16_t send_id;

static inline void end_send(Result res) {
	end_command();
	send_handler(send_id, res);
}

static void send_timeout() {
	auto state_now = send_state;
	if (state_now == SendSmsState::WAIT_INPUT || state_now == SendSmsState::WAIT_ID
			|| state_now == SendSmsState::WAIT_OK) {
		send_state = SendSmsState::DONE;
		end_send(Result::NO_RESPONSE);
	} else if (state_now == SendSmsState::WAIT_OPTIONAL_END) {
		send_state = SendSmsState::DONE;
		end_send(Result::ERROR);
	} else if (state_now == SendSmsState::CANCELING) {

	}
}

/** "on UART sent" callback, which belongs to "send sms" functions */
static BaseType_t send_uart_transferred() {
	BaseType_t task_woken = pdFALSE;
	if (send_state == SendSmsState::DONE) {
		task_woken = invoke_from_task(
				[]() {
					end_send(Result::CORRUPTED_RESPONSE);
				}
		);
	}
	return task_woken;
}

static bool send_listener(rx_buffer_t & rx) {
	auto state_now = send_state;
	if (state_now == SendSmsState::WAIT_INPUT) {
		if (!is_sent()) {
			return false;
		} else if (rx.is_message_corrupted()) {
			// options: "ERROR", "> ", something else
			// cancel SMS sending by transmitting ESC till "ERROR"/"OK"/"> "/something else non-corrupted received
			send_state = SendSmsState::CANCELING;
			send_with_timeout(ESC, length(ESC), send_uart_transferred, RESP_TIMEOUT_ms, send_timeout);
			return true;
		} else if (rx.equals("> ")) {
			send_state = SendSmsState::WAIT_ID;
			send_with_timeout(send_text_buf, send_text_len, SEND_SMS_TIMEOUT_ms, send_timeout);
			return true;
		} else if (rx.equals("ERROR") || rx.starts_with("+CME ERROR:")) {
			send_state = SendSmsState::DONE;
			end_send(Result::ERROR);
			return true;
		} else {
			return false;
		}
	} else if (state_now == SendSmsState::WAIT_ID) {
		// options: "ERROR", "+CMGS: ..."
		if (rx.is_message_corrupted()) {
			send_state = SendSmsState::WAIT_OPTIONAL_END;
			start_response_timeout(RESP_TIMEOUT_ms, send_timeout);
			return true;
		} else if (rx.starts_with("+CMGS:")) {
			char param[6];
			rx.get_param(0, param, 5);
			send_id = atoi(param);
			send_state = SendSmsState::WAIT_OK;
			start_response_timeout(RESP_TIMEOUT_ms, send_timeout);
			return true;
		} else if (rx.equals("ERROR") || rx.starts_with("+CME ERROR:")) {
			send_state = SendSmsState::DONE;
			end_send(Result::ERROR);
			return true;
		} else {
			return false;
		}
	} else if (state_now == SendSmsState::WAIT_OK) {
		send_state = SendSmsState::DONE;
		end_send(Result::OK);
		return true;
	} else if (state_now == SendSmsState::WAIT_OPTIONAL_END) {
		// corrupted message received while waiting for SMS ID
		// if it was "+CMGS: ..." -> expect "OK"; if "ERROR" - timeout will handle
		if (rx.is_message_corrupted() || rx.equals("OK")) {
			send_state = SendSmsState::DONE;
			end_send(Result::CORRUPTED_RESPONSE);
			return true;
		} else  {
			return false;
		}
	} else if (state_now == SendSmsState::CANCELING) {
		if (rx.is_message_corrupted() || rx.equals("> ")) {
			// may be or is "> " - send ESC again if previous ESC is already sent(maybe SIM900 did not receive it)
			if (is_sent()) {
				send_with_timeout(ESC, length(ESC), RESP_TIMEOUT_ms, send_timeout);
			} // else - do nothing, SIM900 will receive prev. ESC
			return true;
		} else if (rx.equals("OK") || rx.equals("ERROR")) {
			// cancellation completed
			send_state = SendSmsState::DONE;
			if (is_sent()) {
				end_send(Result::CORRUPTED_RESPONSE);
			} // else - 'on UART sent' callback will end this
			return true;
		} else {
			// something other is already going on so SMS sending was canceled in SIM900
			stop_timeout();
			send_state = SendSmsState::DONE;
			if (is_sent()) {
				end_send(Result::CORRUPTED_RESPONSE);
			}
			return false; // let other listener to handle message
		}
	}
	return false;
}

void sim900::send_sms(const char * phone, const char * text, void (* callback)(uint16_t id, Result result)) {
	send_handler = callback;
	// put command,
	char * tail = copy("AT+CMGS=\"", tx_buffer);
	tail = copy(phone, tail);
	tail = copy("\"\r", tail);
	uint16_t cmd_len = tail - tx_buffer;
	// put text
	send_text_buf = tail;
	const char * text_end = tx_buffer + (TX_BUFFER_LENGTH - 2); // pointer to ending '\0' + 0x1A
	const char * text_char = text;
	while (*text_char != '\0' && tail < text_end) {
		*tail++ = *text_char++;
	}
	*tail++ = '\0';
	*tail++ = CTRL_Z;
	send_text_len = send_text_buf - tail;
	// send command
	send_state = SendSmsState::WAIT_INPUT;
	begin_command(send_listener);
	send_with_timeout(tx_buffer, cmd_len, RESP_TIMEOUT_ms, send_timeout);
}