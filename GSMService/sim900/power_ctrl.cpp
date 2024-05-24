//
// Created by independent-variable on 3/20/2024.
//
#include "config.h"
#include "power_periph.h"
#include "rcc_utils.h"
#include "./power_ctrl.h"
#include "./uart_periph.h"
#include "./execution.h"

namespace sim900 {
	enum class PowerOnStep : uint8_t {
		OPEN_VBAT = 0, ENABLE_VBAT,
		PRESS_PWR_KEY, /* and enable UART */
		RELEASE_PWR_KEY,
		WAIT_READY_MESSAGE, /* release PWR key if received, else - release PWR key, disable UART and VBAT */
		REPORT_SUCCESS,
		REPORT_FAILURE
	};

	enum class PowerOffStep : uint8_t {
		PRESS_PWR_KEY = uint8_t(PowerOnStep::REPORT_FAILURE) + 1,
		RELEASE_PWR_KEY,
		WAIT_POWER_DOWN_MESSAGE, /* release PWR key when such is received */
		SHORT_VBAT, REPORT_DONE
	};
}

using namespace sim900;

static volatile bool received_message;

static volatile union {
	PowerOnStep on;
	PowerOffStep off;
} next_step;

static volatile union {
	void (* powered_on)(bool success);
	void (* powered_off)();
} callback;

void sim900::init_power_ctrl() {
	GPIO_InitTypeDef pinInitStruct;
	// VBAT switch
	enable_periph_clock(VBAT_SW_PORT);
	disable_vbat(); // VBAT stays off
	pinInitStruct.GPIO_Pin = VBAT_SW_PIN;
	pinInitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	pinInitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(VBAT_SW_PORT, &pinInitStruct);

	// VBAT discharge port
	enable_periph_clock(VBAT_DISCHARGE_PORT);
	open_vbat(); // do not discharge
	pinInitStruct.GPIO_Pin = VBAT_DISCHARGE_PIN;
	pinInitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	pinInitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(VBAT_DISCHARGE_PORT, &pinInitStruct);

	// PWR key port
	enable_periph_clock(PWR_KEY_PORT);
	release_pwr_key(); // do not pull-down
	pinInitStruct.GPIO_Pin = PWR_KEY_PIN;
	pinInitStruct.GPIO_Mode = GPIO_Mode_Out_OD;
	pinInitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(PWR_KEY_PORT, &pinInitStruct);

	TIMER.wait_ticks(1); // give time to VBAT switch and other to settle
	short_vbat();
	TIMER.wait_ms(VBAT_DISCHARGE_TIME);

	next_step.on = PowerOnStep::OPEN_VBAT;
}

static void do_power_on() {
	using namespace sim900;
	switch (next_step.on) {
		case PowerOnStep::OPEN_VBAT:
			open_vbat();
			next_step.on = PowerOnStep::ENABLE_VBAT;
			start_timeout(1, do_power_on);
			break;
		case PowerOnStep::ENABLE_VBAT:
			enable_vbat();
			next_step.on = PowerOnStep::PRESS_PWR_KEY;
			start_timeout(VBAT_SETTIMG_TIME, do_power_on);
			break;
		case PowerOnStep::PRESS_PWR_KEY:
			// "RDY" message may be received while PWR_KEY is still pressed
			// and is received asynchronously - it safer to have next step set
			next_step.on = PowerOnStep::RELEASE_PWR_KEY;
			received_message = false;
			activate_uart();
			press_pwr_key();
			start_timeout(1100U, do_power_on); // hold PWR_KEY > 1s
			break;
		case PowerOnStep::RELEASE_PWR_KEY:
			release_pwr_key();
			next_step.on = PowerOnStep::WAIT_READY_MESSAGE;
			start_timeout(5000, do_power_on); // module starts faster than in 2.2s; invoke callback from thread
			break;
		case PowerOnStep::WAIT_READY_MESSAGE:
			if (received_message) {
				next_step.on = PowerOnStep::REPORT_SUCCESS;
				end_command();
				callback.powered_on(true);
			} else {
				next_step.on = PowerOnStep::REPORT_FAILURE;
				end_command();
				callback.powered_on(false);
			}
			break;
		default:
			// not reachable
			break;
	}
}

static void do_power_off() {
	using namespace sim900;
	switch (next_step.off) {
		case PowerOffStep::PRESS_PWR_KEY:
			// "normal power down" message is received asynchronously - it safer to have next step set
			next_step.off = PowerOffStep::RELEASE_PWR_KEY;
			received_message = false;
			press_pwr_key();
			start_timeout(1100, do_power_off); // hold PWR_KEY > 1s
			break;
		case PowerOffStep::RELEASE_PWR_KEY:
			release_pwr_key();
			next_step.off = PowerOffStep::WAIT_POWER_DOWN_MESSAGE;
			start_timeout(1800, do_power_off); // module stops in 1.7s
			break;
		case PowerOffStep::WAIT_POWER_DOWN_MESSAGE:
			// do not care if message was received or not
			suspend_uart();
			rx_buffer.reset();
			disable_vbat();
			next_step.off = PowerOffStep::SHORT_VBAT;
			start_timeout(1, do_power_off);
			break;
		case PowerOffStep::SHORT_VBAT:
			short_vbat();
			next_step.off = PowerOffStep::REPORT_DONE;
			start_timeout(VBAT_DISCHARGE_TIME, do_power_off); // to invoke callback from thread
			break;
		case PowerOffStep::REPORT_DONE:
			next_step.on = PowerOnStep::OPEN_VBAT;
			end_command();
			callback.powered_off();
			break;
	}
}

static bool receive_ready(rx_buffer_t & rx) {
	if (rx.is_message_corrupted()) {
		return true;
	} else if (rx.equals("RDY")) {
		received_message = true;
		return true;
	} else {
		return false;
	}
}

static bool receive_power_down(rx_buffer_t & rx) {
	if (rx.is_message_ok() && rx.equals("NORMAL POWER DOWN")) {
		received_message = true;
	}
	return true; // do not allow others to receive anything
}

/** @param handler is invoked with "true" if turned on successfully(SIM900 responded),
 * otherwise is invoked with "false"(SIM900 stays powered) */
void sim900::turn_on(void (* handler)(bool)) {
	begin_command(receive_ready);
	callback.powered_on = handler;
	next_step.on = PowerOnStep::OPEN_VBAT;
	do_power_on();
}

/** Guaranties, that will attempt to turned off only if turned on */
void sim900::turn_off(void (* handler)()) {
	begin_command(receive_power_down);
	callback.powered_off = handler;
	next_step.off = PowerOffStep::PRESS_PWR_KEY;
	do_power_off();
}

bool sim900::is_turned_on() {
	PowerOnStep on_step = next_step.on;
	return on_step == PowerOnStep::REPORT_SUCCESS || on_step == PowerOnStep::REPORT_FAILURE;
}

bool sim900::is_turned_off() {
	PowerOnStep on_step = next_step.on;
	return on_step == PowerOnStep::OPEN_VBAT;
}