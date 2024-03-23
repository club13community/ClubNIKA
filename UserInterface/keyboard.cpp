//
// Created by independent-variable on 3/21/2024.
//
#include "stm32f10x.h"
#include "periph_allocation.h"
#include "keyboard_config.h"
#include "keyboard.h"
#include "keyboard_periph_utils.h"
#include "rcc_utils.h"
#include "FreeRTOS.h"
#include "timing.h"

#define us_timer	timing::fine_timer1
#define ms_timer	timing::coarse_timer4
// time between selecting a row and reading it
#define ROW_SETTLING_us	10U
// time between detection of pressed button and first sample
#define DEBOUNCE_250us	3U
// time between samples
#define SAMPLING_250us	3U
// max. number of samples
#define SAMPLE_COUNT_LIMIT	5U

void keyboard::config_periph() {
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);

	// rows
	GPIO_InitTypeDef pinInitStruct = {.GPIO_Speed = GPIO_Speed_2MHz, .GPIO_Mode = GPIO_Mode_Out_OD};
	//PC6, PC7 - R0, R1; PD10, PD11 - R2, R3;
	pinInitStruct.GPIO_Pin=GPIO_Pin_6;
	GPIO_Init(GPIOC, &pinInitStruct);
	pinInitStruct.GPIO_Pin=GPIO_Pin_7;
	GPIO_Init(GPIOC, &pinInitStruct);

	pinInitStruct.GPIO_Pin=GPIO_Pin_10;
	GPIO_Init(GPIOD, &pinInitStruct);
	pinInitStruct.GPIO_Pin=GPIO_Pin_11;
	GPIO_Init(GPIOD, &pinInitStruct);

	// columns
	pinInitStruct = {.GPIO_Mode=GPIO_Mode_IN_FLOATING};
	//PD12-PD15 - C0-C3
	pinInitStruct.GPIO_Pin=GPIO_Pin_12;
	GPIO_Init(GPIOD, &pinInitStruct);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOD, GPIO_PinSource12);

	pinInitStruct.GPIO_Pin=GPIO_Pin_13;
	GPIO_Init(GPIOD, &pinInitStruct);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOD, GPIO_PinSource13);

	pinInitStruct.GPIO_Pin=GPIO_Pin_14;
	GPIO_Init(GPIOD, &pinInitStruct);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOD, GPIO_PinSource14);

	pinInitStruct.GPIO_Pin=GPIO_Pin_15;
	GPIO_Init(GPIOD, &pinInitStruct);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOD, GPIO_PinSource15);
}

enum class State : uint8_t {
	WAIT_PRESS,
	DEBOUNCE, /* debounce time after presses button was detected */
	SAMPLING, /* samples keyboard several time - button is assumed pressed if N consequent samples show this */
	WAIT_FAST_RELEASE, /* and run timer to detect long press */
	WAIT_LONG_RELEASE
};

static volatile State state = State::WAIT_PRESS;

/** Expects that all rows are deselected */
static uint16_t read_keyboard() {
	uint16_t kb = 0;
	using namespace keyboard;

	select_row_0();
	us_timer.wait_us(ROW_SETTLING_us);
	kb |= (uint16_t) read_columns() << 0;
	deselect_row_0();

	select_row_1();
	us_timer.wait_us(ROW_SETTLING_us);
	kb |= (uint16_t) read_columns() << 4;
	deselect_row_1();

	select_row_2();
	us_timer.wait_us(ROW_SETTLING_us);
	kb |= (uint16_t) read_columns() << 8;
	deselect_row_2();

	select_row_3();
	us_timer.wait_us(ROW_SETTLING_us);
	kb |= (uint16_t) read_columns() << 12;
	deselect_row_3();

	return kb;
}

enum class Trigger : uint8_t {
	TIMER = 1U,
	KEYBOARD
};

struct Key {
	uint8_t row;
	uint8_t col;

	volatile Key & operator=(const Key & other) volatile {
		row = other.row;
		col = other.col;
		return *this;
	}
};

static uint16_t samples[3], sample_count;
volatile Key active_key;

static Key get_key(uint16_t kb) {
	// search first non-zero bit
	uint16_t bit_msk = 0x8000U, bit_ind = 15;
	while (!(kb & bit_msk)) {
		bit_msk >>= 1;
		bit_ind -= 1;
	}
	uint8_t row = bit_ind >> 2; // key_ind / 4
	uint8_t col = bit_ind - (row << 2); // key_ind % 4
	return {.row = row, .col = col};
}

/** Reconfigures EXTI to detect release of specified key.
 * Expects, that no row is selected and EXTI for all columns are disabled before invocation */
static void detect_release_of(Key key) {
	using namespace keyboard;
	select_row(key.row);
	exti_on_release_for(key.col);
	us_timer.wait_us(ROW_SETTLING_us);
	clear_exti_flag_for(key.col);
	if (!is_pressed(key.col)) {
		set_exti_flag_for(key.col);
	}
	enable_exti_for(key.col);
}

/** Reconfigures EXTI to detect pressing of any key. */
static void detect_press_of_any() {
	using namespace keyboard;
	exti_on_press_for_all();
	select_all_rows();
	clear_exti_flags_for_all();
	enable_exti_for_all();
}

void handle_timeout() {

}

void handle_exti() {

}

static void handle_state(Trigger trigger) {
	using namespace keyboard;
	switch (state) {
		case State::WAIT_PRESS:
			// the only possible source is keyboard
			state = State::DEBOUNCE;
			deselect_all_rows();
			disable_exti_for_all();
			ms_timer.invoke_in_ticks(DEBOUNCE_250us, handle_timeout);
			break;
		case State::DEBOUNCE:
			// the only possible source is timer
			state = State::SAMPLING;
			samples[0] = read_keyboard();
			sample_count = 1;
			ms_timer.invoke_in_ticks(SAMPLING_250us, handle_timeout);
			break;
		case State::SAMPLING:
			// the only possible source is timer
			samples[2] = samples[1];
			samples[1] = samples[0];
			samples[0] = read_keyboard();
			sample_count++;
			if (sample_count >= 3) {
				uint16_t stable_keyboard = samples[0] & samples[1] & samples[2];
				if (stable_keyboard) {
					// some button is pressed during 3 samples
					state = State::WAIT_FAST_RELEASE;
					Key key = get_key(stable_keyboard);
					active_key = key;
					detect_release_of(key);
					ms_timer.invoke_in_ms(SAMPLING_250us, handle_timeout);
				} else if (sample_count >= SAMPLE_COUNT_LIMIT) {
					// no button is stably pressed during sampling period - wait for pressing any key
					state = State::WAIT_PRESS;
					detect_press_of_any();
				}
			}
			break;
		case State::WAIT_FAST_RELEASE:
			if (trigger == Trigger::TIMER) {
				// timeout for 'short tap' expired - now it is 'long tap'
				state = State::WAIT_LONG_RELEASE;
				//todo: send event
			} else {
				// key released
				state = State::WAIT_PRESS;
				detect_press_of_any();
				// todo: send event
			}
			break;
		case State::WAIT_LONG_RELEASE:
			state = State::WAIT_PRESS;
			detect_press_of_any();
			break;
	}
}

