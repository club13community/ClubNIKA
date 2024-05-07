//
// Created by independent-variable on 3/20/2024.
//
#include <stdint.h>
#include <exception>
#include "./config.h"
#include "./sim900_power_ctrl.h"
#include "./sim900_periph_utils.h"
#include "rcc_utils.h"
#include "./string_utils.h"

enum class PowerOnStep : uint8_t {
	OPEN_VBAT, ENABLE_VBAT,
	PRESS_PWR_KEY, /* and enable UART */
	RELEASE_PWR_KEY,
	WAIT_READY_MESSAGE, /* release PWR key if received, else - release PWR key, disable UART and VBAT */
	REPORT_SUCCESS,
	SHORT_VBAT, REPORT_FAILURE
};

enum class PowerOffStep : uint8_t {
	PRESS_PWR_KEY = uint8_t(PowerOnStep::REPORT_FAILURE),
	RELEASE_PWR_KEY,
	WAIT_POWER_DOWN_MESSAGE, /* release PWR key when such is received */
	SHORT_VBAT, REPORT_DONE
};

union PowerStatus {
	PowerOnStep on_step;
	PowerOffStep off_step;
	uint8_t as_byte;

	PowerStatus() : off_step(PowerOffStep::REPORT_DONE) {}
	PowerStatus(PowerStatus & other) : as_byte(other.as_byte) {}
	PowerStatus(uint8_t byte_value) : as_byte(byte_value) {}

	bool is_turned_on() {
		return on_step == PowerOnStep::REPORT_SUCCESS;
	}

	bool is_turned_off() {
		return off_step == PowerOffStep::REPORT_DONE || on_step == PowerOnStep::REPORT_FAILURE;
	}
};

static volatile PowerStatus power;
static volatile bool received_message;
static void (* volatile power_on_callback)(bool);
static void (* volatile power_off_callback)();

static void do_power_on() {
	using namespace sim900;
	switch (power.on_step) {
		case PowerOnStep::OPEN_VBAT:
			open_vbat();
			power.on_step = PowerOnStep::ENABLE_VBAT;
			TIMER.invoke_in_ticks(1, do_power_on);
			break;
		case PowerOnStep::ENABLE_VBAT:
			enable_vbat();
			power.on_step = PowerOnStep::PRESS_PWR_KEY;
			TIMER.invoke_in_ms(VBAT_SETTIMG_TIME, do_power_on);
			break;
		case PowerOnStep::PRESS_PWR_KEY:
			// "RDY" message may be received while PWR_KEY is still pressed
			// and is received asynchronously - it safer to have next step set
			power.on_step = PowerOnStep::RELEASE_PWR_KEY;
			received_message = false;
			activate_uart();
			press_pwr_key();
			TIMER.invoke_in_ms(1100, do_power_on); // hold PWR_KEY > 1s
			break;
		case PowerOnStep::RELEASE_PWR_KEY:
			release_pwr_key();
			power.on_step = PowerOnStep::WAIT_READY_MESSAGE;
			TIMER.invoke_in_ms(5000, do_power_on); // module starts faster than in 2.2s
			break;
		case PowerOnStep::WAIT_READY_MESSAGE:
			if (received_message) {
				power.on_step = PowerOnStep::REPORT_SUCCESS;
				power_on_callback(true);
			} else {
				suspend_uart();
				disable_vbat();
				power.on_step = PowerOnStep::SHORT_VBAT;
				TIMER.invoke_in_ticks(1, do_power_on);
			}
			break;
		case PowerOnStep::SHORT_VBAT:
			short_vbat();
			power.on_step = PowerOnStep::REPORT_FAILURE;
			TIMER.invoke_in_ms(VBAT_DISCHARGE_TIME, do_power_on);
			break;
		case PowerOnStep::REPORT_FAILURE:
			power_on_callback(false);
			break;
		default: throw std::exception();
	}
}

static void do_power_off() {
	using namespace sim900;
	switch (power.off_step) {
		case PowerOffStep::PRESS_PWR_KEY:
			// "normal power down" message is received asynchronously - it safer to have next step set
			power.off_step = PowerOffStep::RELEASE_PWR_KEY;
			received_message = false;
			press_pwr_key();
			TIMER.invoke_in_ms(1100, do_power_off); // hold PWR_KEY > 1s
			break;
		case PowerOffStep::RELEASE_PWR_KEY:
			release_pwr_key();
			power.off_step = PowerOffStep::WAIT_POWER_DOWN_MESSAGE;
			TIMER.invoke_in_ms(1800, do_power_off); // module stops in 1.7s
			break;
		case PowerOffStep::WAIT_POWER_DOWN_MESSAGE:
			// do not care if message was received or not
			suspend_uart();
			disable_vbat();
			power.off_step = PowerOffStep::SHORT_VBAT;
			TIMER.invoke_in_ticks(1, do_power_off);
			break;
		case PowerOffStep::SHORT_VBAT:
			short_vbat();
			power.off_step = PowerOffStep::REPORT_DONE;
			TIMER.invoke_in_ms(VBAT_DISCHARGE_TIME, do_power_off);
			break;
		case PowerOffStep::REPORT_DONE:
			power_off_callback();
			break;
		default: throw std::exception();
	}
}

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
}

bool sim900::handle_power_ctrl_message(char * message, uint16_t length) {
	PowerStatus current = const_cast<PowerStatus &>(power);
	if ((current.on_step == PowerOnStep::RELEASE_PWR_KEY || current.on_step == PowerOnStep::WAIT_READY_MESSAGE)
		&& length == 3 && equals(message, "RDY", length)) {
		received_message = true;
		return true;
	}
	if ((current.off_step == PowerOffStep::RELEASE_PWR_KEY || current.off_step == PowerOffStep::WAIT_POWER_DOWN_MESSAGE)
		&& length == 17 && equals(message, "NORMAL POWER DOWN", length)) {
		received_message = true;
		return true;
	}
	return false;
}

/** Guaranties, that will attempt to turned on only if turned off.
 * @param callback is invoked with "true" if turned on successfully,
 * otherwise is invoked with "false" and SIM900 stays turned off */
void sim900::turn_on(void (* callback)(bool)) {
	uint8_t * const power_address = const_cast<uint8_t *>(&power.as_byte);
	PowerStatus current;
	do {
		current = __LDREXB(power_address);
		if (!current.is_turned_off()) {
			__CLREX();
			throw std::exception();
		}
		current.on_step = PowerOnStep::OPEN_VBAT;
	} while (__STREXB(current.as_byte, power_address));
	power_on_callback = callback;
	do_power_on();
}

/** Guaranties, that will attempt to turned off only if turned on */
void sim900::turn_off(void (* callback)()) {
	uint8_t * const power_address = const_cast<uint8_t *>(&power.as_byte);
	PowerStatus current;
	do {
		current = __LDREXB(power_address);
		if (!current.is_turned_on()) {
			__CLREX();
			throw std::exception();
		}
		current.off_step = PowerOffStep::PRESS_PWR_KEY;
	} while (__STREXB(current.as_byte, power_address));
	power_off_callback = callback;
	do_power_off();
}

bool sim900::is_turned_on() {
	PowerStatus current = const_cast<PowerStatus &>(power);
	return current.is_turned_on();
}

bool sim900::is_turned_off() {
	PowerStatus current = const_cast<PowerStatus &>(power);
	return current.is_turned_off();
}