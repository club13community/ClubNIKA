//
// Created by independent-variable on 5/14/2024.
//
#include <stdlib.h>
#include "sim900.h"
#include "sim900_callbacks.h"
#include "./sms_ctrl.h"
#include "./execution.h"
#include "./utils.h"
#include "./config.h"
#include "./ctrl_templates.h"

using namespace sim900;

#define CTRL_Z	( (char)0x1AU )
#define ESC		( (char)0x1BU )

namespace sim900 {
	enum class SendSmsState {
		SENDING_CMD, WAITING_INPUT, SENDING_TEXT, WAITING_ID, WAITING_OK, FAILED
	};
}

static void (* volatile send_handler)(uint16_t, Result);
static char * volatile send_text_buf;
static volatile uint16_t send_text_len;
static volatile uint16_t send_id;
static volatile SendSmsState send_state;

static void end_send(Result res) {
	end_command();
	send_handler(send_id, res);
}

/** "on UART sent" callback, which belongs to "send sms" functions */
static void send_transferred() {
	SendSmsState state_now = send_state;
	if (state_now == SendSmsState::SENDING_CMD) {
		send_state = SendSmsState::WAITING_INPUT;
	} else if (state_now == SendSmsState::SENDING_TEXT) {
		send_state = SendSmsState::WAITING_ID;
	} else if (state_now == SendSmsState::FAILED) {
		end_send(Result::ERROR);
	}
}

static bool send_listener(rx_buffer_t & rx) {
	SendSmsState state_now = send_state;
	if (rx.is_message_corrupted()) {
		if (state_now == SendSmsState::SENDING_CMD) {
			// not a response to "send SMS" command
			return false;
		} else {
			if (state_now == SendSmsState::SENDING_TEXT) {
				send_state = SendSmsState::FAILED; // 'on transfer' will end
			} else { // WAITING_INPUT or WAITING_OK or WAITING_ID
				end_send(Result::CORRUPTED_RESPONSE); // sending is not ongoing - end here
			}
			return true;
		}
	}
	if (state_now == SendSmsState::FAILED) {
		return false;
	} else if (rx.equals("> ")) {
		send_state = SendSmsState::SENDING_TEXT;
		send_with_timeout(send_text_buf, send_text_len, send_transferred,
						  SEND_SMS_TIMEOUT_s * 1000U, end_on_timeout<end_send>);
		return true;
	} else if (rx.starts_with("+CMGS:")) {
		char param[6];
		rx.get_param(0, param, 5);
		send_id = atoi(param);
		send_state = SendSmsState::WAITING_OK;
		start_response_timeout(RESP_TIMEOUT_ms, end_on_timeout<end_send>);
		return true;
	} else if (rx.equals("OK")) {
		// if not waiting "OK" -> message with SMS ID was lost
		Result res = send_state == SendSmsState::WAITING_OK ? Result::OK : Result::CORRUPTED_RESPONSE;
		end_send(res);
		return true;
	} else if (rx.equals("ERROR") || rx.starts_with("+CME ERROR:")) {
		// may be received during any state
		SendSmsState state_now = send_state;
		if (state_now == SendSmsState::SENDING_CMD || state_now == SendSmsState::SENDING_TEXT) {
			send_state = SendSmsState::FAILED; // 'on transferred' will end
		} else {
			end_send(Result::ERROR);
		}
		return true;
	}
	return false;
}

void sim900::send_sms(const char * phone, const char * text, void (* callback)(uint16_t id, Result result)) {
	send_handler = callback;

	// put command
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
	send_text_len = tail - send_text_buf;

	// send command
	send_state = SendSmsState::SENDING_CMD;
	begin_command(send_listener);
	send_with_timeout(tx_buffer, cmd_len, send_transferred, RESP_TIMEOUT_ms, end_on_timeout<end_send>);
}

static void (* volatile delete_sent_handler)(Result);

static void end_delete_sent(Result res) {
	end_command();
	delete_sent_handler(res);
};

void sim900::delete_sent_sms(void (* callback)(Result result)) {
	static constexpr const char * cmd = "AT+CMGDA=\"DEL SENT\"\r";

	delete_sent_handler = callback;
	start_execute<end_delete_sent>(cmd, length(cmd), RESP_TIMEOUT_ms);
}

static void (* volatile delete_received_handler)(Result);

static void end_delete_received(Result res) {
	end_command();
	delete_received_handler(res);
};

void sim900::delete_received_sms(void (* callback)(Result result)) {
	static constexpr const char * cmd = "AT+CMGDA=\"DEL INBOX\"\r";

	delete_received_handler = callback;
	start_execute<end_delete_received>(cmd, length(cmd), RESP_TIMEOUT_ms);
}

static void (* volatile delete_all_handler)(Result);

static void end_delete_all(Result res) {
	end_command();
	delete_all_handler(res);
};

void sim900::delete_all_sms(void (* callback)(Result result)) {
	static constexpr const char * cmd = "AT+CMGDA=\"DEL ALL\"";

	delete_all_handler = callback;
	start_execute<end_delete_all>(cmd, length(cmd), RESP_TIMEOUT_ms);
}

bool sim900::incoming_sms_listener(rx_buffer_t & rx) {
	if (rx.is_message_corrupted()) {
		return false;
	}
	if (!rx.starts_with("+CMTI:")) {
		return false;
	}
	char param[4];
	rx.get_param(1, param, 3);
	uint16_t id = atoi(param);
	on_sms_received(id);
	return true;
}