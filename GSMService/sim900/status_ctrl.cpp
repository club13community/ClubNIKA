//
// Created by independent-variable on 5/14/2024.
//
#include <string.h>
#include "sim900.h"
#include "./execution.h"
#include "./ctrl_templates.h"
#include "./utils.h"

using namespace sim900;

static void (* volatile card_status_callback)(CardStatus, Result);
static volatile CardStatus card_status;

static void parse_card_status(rx_buffer_t & rx) {
	char param[13];
	rx.get_param(0, param, 13);
	if (strcmp("READY", param) == 0) {
		card_status = CardStatus::READY;
	} else if (strcmp("NOT READY", param) == 0) {
		card_status = CardStatus::ERROR;
	} else if (strcmp("NOT INSERTED", param) == 0) {
		card_status = CardStatus::ABSENT;
	} else {
		card_status = CardStatus::LOCKED;
	}
}

static void end_card_status(Result res) {
	end_command();
	if (res == Result::OK) {
		card_status_callback(card_status, res);
	} else if (res == Result::ERROR) {
		card_status_callback(CardStatus::ABSENT, Result::OK); // checked on practice
	} else {
		card_status_callback(card_status, res);
	}
}

void sim900::get_card_status(void (* callback)(CardStatus status, Result result)) {
	static const char * cmd = "AT+CPIN?\r";
	static const char * CPIN = "+CPIN:";

	card_status_callback = callback;
	start_get_info<CPIN, parse_card_status, end_card_status>(cmd, length(cmd), RESP_TIMEOUT_ms);
}

static void (* volatile signal_strength_handler)(uint8_t, Result);
static volatile uint8_t signal_strength;

static void parse_signal_strength(rx_buffer_t & rx) {
	char param[3];
	uint16_t len = rx.get_param(0, param, 2);
	uint8_t code = param[0] & 0x0F;
	if (len > 1) {
		code = code * 10 + (param[1] & 0x0F);
	}
	if (code == 99U) {
		signal_strength = 0;
	} else {
		// (code * 3.19 * 1024)/1024 + 0.5 + 1 = (code * 3270) / 1024 + 1/2 + 2/2 =
		// = (code * 3270 + 3 * 1024)/(2 * 1024)
		signal_strength = (code * 6540U + 3072) >> 11;
	}
}

static void end_signal_strength(Result res) {
	end_command();
	signal_strength_handler(signal_strength, res);
}

void sim900::get_signal_strength(void (* callback)(uint8_t signal_pct, Result result)) {
	static const char * cmd = "AT+CSQ\r";
	static const char * CSQ = "+CSQ:";

	signal_strength_handler = callback;
	start_get_info<CSQ, parse_signal_strength, end_signal_strength>(cmd, length(cmd), RESP_TIMEOUT_ms);
}

static void (* volatile registration_handler)(Registration, Result);
static volatile Registration registration;

static void parse_registration(rx_buffer_t & rx) {
	char param[2];
	rx.get_param(1, param, 1);
	if (param[0] == '1' || param[0] == '5') {
		registration = Registration::DONE;
	} else if (param[0] == '2' || param[0] == '4') {
		registration = Registration::ONGOING;
	} else {
		registration = Registration::FAILED;
	}
}

static void end_registration(Result res) {
	end_command();
	registration_handler(registration, res);
}

void sim900::get_registration(void (* callback)(Registration registration, Result result)) {
	static const char * cmd = "AT+CREG?\r";
	static const char * CREG = "+CREG:";

	registration_handler = callback;
	start_get_info<CREG, parse_registration, end_registration>(cmd, length(cmd), RESP_TIMEOUT_ms);
}