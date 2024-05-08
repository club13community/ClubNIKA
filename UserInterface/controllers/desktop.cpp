//
// Created by independent-variable on 4/5/2024.
//
#include "../controllers.h"
#include "../display.h"
#include "../symbols.h"

namespace user_interface {
	class Desktop : public Controller {
	protected:
		void activate(bool init) override;
		void handle(keyboard::Button button, keyboard::Event event) override;
	};

	static Desktop desktop_inst;
	Controller * const desktop = &desktop_inst;
}

using namespace user_interface;

static const char battery = '\0', signal = '\1', card = '\2', U = '\3';

void Desktop::activate(bool init) {
	disp.clear()
			.define(battery, symbol::battery)
			.define(signal, symbol::signal_level)
			.define(card, symbol::card)
			.define(U, symbol::ua_U)
			.set_cursor(0, 0);

	disp << battery << 10 << '%' << ' ' << signal << 50 << '%' << ' ' << "SD" << card << '+' << endl;
	disp << "A:AKT" << U << "B.  B:MENU";
}

void Desktop::handle(keyboard::Button button, keyboard::Event event) {
	using keyboard::Button, keyboard::Event;
	if (button == Button::B && event == Event::CLICK) {
		invoke(menu);
	}
}
