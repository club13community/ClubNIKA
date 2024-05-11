//
// Created by independent-variable on 5/10/2024.
//
#include "../controllers.h"
#include "../display.h"
#include "../symbols.h"
#include "settings.h"
#include "alarm.h"
#include "UserInterface.h"

namespace user_interface {
	class AlarmEnabler : public Controller {
	private:
		static constexpr uint8_t timer_pos = 9;
		uint16_t delay;
	private:
		void print_delay();
	protected:
		void activate(bool init) override;
		void handle(keyboard::Button button, keyboard::Event event) override;
		void delay_elapsed() override;
	};

	static AlarmEnabler enabler;
	Controller * const alarm_enabler = &enabler;
}

using namespace user_interface;

void AlarmEnabler::activate(bool init) {
	if (init) {
		delay = get_alarm_delay_s();
		handle_ui_inactivity(false); // display is lighted up during all delay before arming
		start_delay(1000);
	}

	disp.clear()
			.define('\1', symbol::ua_D).define('\2', symbol::ua_U)
			.set_cursor(0, 0).print("\1O AKT\2B.")
			.set_cursor(1, 3).print("A:BI\1MIHA");
	print_delay();
}

void AlarmEnabler::handle(keyboard::Button button, keyboard::Event event) {
	using keyboard::Button, keyboard::Event;
	if (button == Button::A && event == Event::CLICK) {
		// cancel
		yield();
	}
}

void AlarmEnabler::delay_elapsed() {
	delay -= 1;
	if (delay == 0) {
		alarm::arm();
		user_interface::alarm_armed(); // this will change controller
	} else {
		print_delay();
		start_delay(1000);
	}
}

void AlarmEnabler::print_delay() {
	char text[5] = "   c";
	char * dig = text + 2;
	uint16_t val = delay;
	do {
		*dig-- = 0x30 | val % 10;
		val /= 10;
	} while (val);
	disp.set_cursor(0, timer_pos).print(text);
}