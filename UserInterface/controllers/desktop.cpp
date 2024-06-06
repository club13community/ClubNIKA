//
// Created by independent-variable on 4/5/2024.
//
#include "../controllers.h"
#include "../display.h"
#include "../symbols.h"
#include "GSMService.h"

#define SIGNAL_REFRESH_TIME_s	5U

namespace user_interface {
	class Desktop : public Controller {
	private:
		uint8_t gsm_signal;
	protected:
		void activate(bool init) override;
		void handle(keyboard::Button button, keyboard::Event event) override;
		void delay_elapsed() override;
	};

	static Desktop desktop_inst;
	Controller * const desktop = &desktop_inst;
}

using namespace user_interface;

static const char battery = '\0', signal = '\1', card = '\2', U = '\3';

void Desktop::activate(bool init) {
	if (init) {
		gsm_signal = gsm::get_signal_strength();
		start_delay(SIGNAL_REFRESH_TIME_s * 1000U);
		if (gsm_signal == 0) {
			handle_ui_inactivity(false); // to keep display lighted up while no GSM signal
		}
	}

	disp.clear()
			.define(battery, symbol::battery)
			.define(signal, symbol::signal_level)
			.define(card, symbol::card)
			.define(U, symbol::ua_U)
			.set_cursor(0, 0);

	disp.set_cursor(0, 0) << battery << 10 << '%';
	disp.set_cursor(0, 6) << signal << (int)gsm_signal << '%';
	disp.set_cursor(0, 12) << "SD" << card << '+';

	disp.set_cursor(1, 0) << "A:AKT" << U << "B.";
	disp.set_cursor(1, 10) << "B:MENU";
}

void Desktop::handle(keyboard::Button button, keyboard::Event event) {
	using keyboard::Button, keyboard::Event;
	if (button == Button::A && event == Event::CLICK) {
		invoke(alarm_enabler);
	} else if (button == Button::B && event == Event::CLICK) {
		invoke(menu);
	}
}

void Desktop::delay_elapsed() {
	uint8_t new_signal = gsm::get_signal_strength();
	disp.set_cursor(0, 7).clear(4).print((int)new_signal).print('%');
	if (new_signal == 0 && gsm_signal > 0) {
		// signal disappeared - light up display
		handle_ui_inactivity(false);
		disp.light_up();
	} else if (new_signal > 0 && gsm_signal == 0) {
		// signal restored
		handle_ui_inactivity(true);
		// will put out display automatically on inactivity
	}
	gsm_signal = new_signal;
}
