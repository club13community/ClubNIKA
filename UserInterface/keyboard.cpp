//
// Created by independent-variable on 3/21/2024.
//
#include "stm32f10x.h"
#include "periph_allocation.h"
#include "./keyboard_config.h"
#include "keyboard.h"
#include "./keyboard_periph.h"
#include "./ui_private.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timing.h"

#define us_timer	timing::fine_timer1
#define ms_timer	timing::coarse_timer4

namespace keyboard {
	enum class State : uint8_t {
		WAIT_PRESS = 1U,
		DEBOUNCE, /* debounce time after presses button was detected */
		SAMPLING, /* samples keyboard several time - button is assumed pressed if N consequent samples show this */
		WAIT_FAST_RELEASE, /* and run timer to detect long press */
		WAIT_LONG_RELEASE
	};

	enum class Trigger : uint8_t {
		TIMER = 1U,
		KEYBOARD
	};

	struct Notice {
		Trigger trigger;
		State state;

		Notice(Trigger trigger, State state) : trigger(trigger), state(state) {}

		Notice(uint32_t val) {
			state = (State)(val & 0xFF);
			trigger = (Trigger)(val >> 8);
		}

		operator uint32_t () {
			return ((uint16_t)trigger << 8) | (uint8_t)state;
		}
	};

	struct Key {
		uint8_t row;
		uint8_t col;
	};
}

using namespace keyboard;

void keyboard::init_periph() {
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	// rows
	deselect_all_rows();
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

	NVIC_InitTypeDef nvic_conf = {0};
	nvic_conf.NVIC_IRQChannel = EXTI15_10_IRQn;
	nvic_conf.NVIC_IRQChannelPreemptionPriority = KEYBOARD_IRQ_PRIORITY;
	nvic_conf.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic_conf);

	exti_on_press_for_all();
}

/** Expects that all rows are deselected */
static uint16_t read_keyboard() {
	uint16_t kb = 0;

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

static Key get_key(uint16_t kb) {
	// search first non-zero bit
	uint16_t bit_msk = 0x8000U, bit_ind = 15;
	while (!(kb & bit_msk)) {
		bit_msk >>= 1;
		bit_ind -= 1;
	}
	uint8_t row = bit_ind >> 2; // key_ind / 4
	uint8_t col = bit_ind - (row << 2); // key_ind % 4
	return {row, col};
}

/** Reconfigures EXTI to detect release of specified key.
 * Expects, that no row is selected and EXTI for all columns are disabled before invocation */
static void detect_release_of(Key key) {
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
	exti_on_press_for_all();
	clear_exti_flags_for_all();
	enable_exti_for_all();
	select_all_rows();
}

static volatile State state;
static TaskHandle_t service;
static StackType_t service_stack[STACK_SIZE];
static StaticTask_t service_ctrl;

void handle_timeout() {
	State current = state;
	BaseType_t hpw = pdFALSE;
	if (current == State::DEBOUNCE || current == State::SAMPLING || current == State::WAIT_FAST_RELEASE) {
		xTaskNotifyFromISR(service, Notice(Trigger::TIMER, current), eSetValueWithoutOverwrite, &hpw);
	}
	portYIELD_FROM_ISR(hpw);
}

void keyboard::exti_isr() {
	State current = state;
	BaseType_t hpw = pdFALSE;
	if (current == State::WAIT_PRESS || current == State::WAIT_FAST_RELEASE || current == State::WAIT_LONG_RELEASE) {
		xTaskNotifyFromISR(service, Notice(Trigger::KEYBOARD, current), eSetValueWithoutOverwrite, &hpw);
	}
	clear_exti_flags_for_all();
	portYIELD_FROM_ISR(hpw);
}

static volatile Key active_key;

static const Button layout[4][4] = {
		{Button::N1,   Button::N2, Button::N3,    Button::A},
		{Button::N4,   Button::N5, Button::N6,    Button::B},
		{Button::N7,   Button::N8, Button::N9,    Button::C},
		{Button::STAR, Button::N0, Button::POUND, Button::D}
};

static void add_active_key_event(Event event) {
	Button button = layout[active_key.row][active_key.col];
	ButtonEvent buttonEvent = ButtonEvent(button, event);
	user_interface::handle(buttonEvent);
}

static uint16_t samples[3], sample_count;

static void handle_state(Trigger trigger) {
	using namespace keyboard;
	uint16_t stable_keyboard = 0;
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
				stable_keyboard = samples[0] & samples[1] & samples[2];
			}
			if (stable_keyboard) {
				// some button is pressed during 3 samples
				state = State::WAIT_FAST_RELEASE;
				Key key = get_key(stable_keyboard);
				active_key.row = key.row;
				active_key.col = key.col;
				detect_release_of(key);
				ms_timer.invoke_in_ms(FAST_RELEASE_ms, handle_timeout);
			} else if (sample_count >= SAMPLE_COUNT_LIMIT) {
				// no button is stably pressed during sampling period - wait for pressing any key
				state = State::WAIT_PRESS;
				detect_press_of_any();
			} else {
				ms_timer.invoke_in_ticks(SAMPLING_250us, handle_timeout);
			}
			break;
		case State::WAIT_FAST_RELEASE:
			if (trigger == Trigger::TIMER) {
				// timeout for 'click' expired - now it is 'long press'
				state = State::WAIT_LONG_RELEASE;
				add_active_key_event(Event::LONG_PRESS);
			} else {
				// key released
				state = State::WAIT_PRESS;
				ms_timer.stop();
				add_active_key_event(Event::CLICK);
			}
			detect_press_of_any();
			break;
		case State::WAIT_LONG_RELEASE:
			state = State::WAIT_PRESS;
			detect_press_of_any();
			break;
	}
}

static void service_keyboard(void * args) {
	while (true) {
		uint32_t val;
		while (pdFALSE == xTaskNotifyWait(0, 0, &val, portMAX_DELAY));
		Notice notice = val;
		if (state == notice.state) {
			handle_state(notice.trigger);
		}
	}
}

void keyboard::start() {
	service = xTaskCreateStatic(service_keyboard, "kb. serv.", STACK_SIZE, nullptr,
								KEYBOARD_SERVICE_PRIORITY, service_stack, &service_ctrl);
	state = State::WAIT_PRESS;
	clear_exti_flags_for_all();
	enable_exti_for_all();
	select_all_rows();
}