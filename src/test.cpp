//
// Created by independent-variable on 3/28/2024.
//
#include "timing.h"
#include "../UserInterface/lcd_periph.h"

static void init_led() {
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	lcd::led_off();
	GPIO_InitTypeDef pinInitStruct;
	pinInitStruct.GPIO_Speed=GPIO_Speed_2MHz;
	pinInitStruct.GPIO_Mode=GPIO_Mode_Out_PP;
	pinInitStruct.GPIO_Pin = LED_PIN;
	GPIO_Init(RS_RW_E_LED_PORT, &pinInitStruct);
}

//--------------------------

void test_timer_wait() {
	init_led();

	while(true) {
		timing::coarse_timer1.wait_ms(1000);
		lcd::led_on();
		timing::coarse_timer1.wait_ms(1000);
		lcd::led_off();
	}
}

//--------------------------

static void turn_off_and_reschedule();
static void turn_on_and_reschedule() {
	lcd::led_on();
	timing::coarse_timer1.invoke_in_ms(500, turn_off_and_reschedule);
}

static void turn_off_and_reschedule() {
	lcd::led_off();
	timing::coarse_timer1.invoke_in_ms(1000, turn_on_and_reschedule);
}

void test_timer_invoke_once() {
	init_led();

	timing::coarse_timer1.invoke_in_ms(1000, turn_on_and_reschedule);
	while (true);

}

//--------------------------
static volatile uint8_t counter;

static void blink() {
	counter++;
	if (counter % 2) {
		lcd::led_on();
	} else {
		lcd::led_off();
	}
	if (counter == 6) {
		timing::coarse_timer1.stop();
	}
}

void test_timer_invoke_repeatedly_and_stop() {
	init_led();
	counter = 0;
	timing::coarse_timer1.every_ms_invoke(1000, blink);
	while (true);
}

//-------------------------
static void start_blinking() {
	timing::coarse_timer1.every_ms_invoke(800, blink);
}

void test_timer_start_blink_while_delay() {
	init_led();
	timing::coarse_timer1.invoke_in_ms(10000, lcd::led_on); // in 10 sec.
	timing::coarse_timer2.invoke_in_ms(5000, start_blinking);
	while (true);
}