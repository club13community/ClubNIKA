//
// Created by independent-variable on 4/7/2024.
//
#include "../controllers.h"
#include "../display.h"
#include "../symbols.h"

static const char up = '\0', down = '\1', enter = '\2', exit = '\3', U = '\4', P = '\5';

namespace user_interface {
	enum class Activation : uint8_t {NONE = 0, CLOSE, OPEN};

	class ZoneConfigurer : public Controller {
	private:
		uint8_t zone = 0;
		const uint8_t first_zone = 0, last_zone = 7;
		const uint8_t up_pos = 0, down_pos = 4;
		Activation activation[8] = {Activation::NONE, Activation::NONE, Activation::NONE, Activation::NONE,
									Activation::NONE, Activation::NONE, Activation::NONE, Activation::NONE};

		/** Expects, that cursor is on the 2nd row. 'Overflows' row, so cursor is on the 1st line after return */
		inline void show_activation() {
			switch (activation[zone]) {
				case Activation::NONE:
					disp[3] << "HE AKT" << U << "BHA   ";
					break;
				case Activation::CLOSE:
					disp[3] << "AKT. " << P << 'P' << U << " K3  ";
					break;
				case Activation::OPEN:
					disp[3] << "AKT. " << P << 'P' << U << " PO3P";
					break;
			}
		}

		/** Expects, that cursor is on the 2nd row. 'Overflows' row, so cursor is on the 1st line after return */
		inline void show_zone() {
			disp[0] << zone + 1 << ") ";
			show_activation();
		}
	public:
		void activate() override ;
		void handle(keyboard::Button button, keyboard::Event event) override ;
	};

	static ZoneConfigurer configurer_inst;
	Controller * const zone_configurer = &configurer_inst;
}

using namespace user_interface;

void ZoneConfigurer::activate() {
	disp
			.put_out_on_inactivity()
			.light_up()
			.clear()
			.define(up, symbol::up)
			.define(down, symbol::down)
			.define(enter, symbol::enter)
			.define(exit, symbol::exit)
			.define(U, symbol::ua_U)
			.define(P, symbol::ua_P)
			.set_cursor(0, 0);
	// show 1st row
	if (zone != first_zone) {
		disp[up_pos] << "A:" << up;
	}
	if (zone != last_zone) {
		disp[down_pos] << "B:" << down;
	}
	disp[9] << "C:" << enter << ' ' << "D:" << exit;
	// show 2nd row
	disp.set_cursor(1, 0);
	show_zone();
	disp.set_cursor(1, 0);
}

void ZoneConfigurer::handle(keyboard::Button button, keyboard::Event event) {
	using keyboard::Button, keyboard::Event;
	if (button == Button::A && event == Event::CLICK) {
		// move up
		if (zone == first_zone) {
			return;
		}
		uint8_t prev_zone = zone--;
		show_zone();
		if (zone == first_zone) {
			// hide 'up'
			disp.set_cursor(0, up_pos).print("   ");
		} else if (prev_zone == last_zone) {
			// show 'down'
			disp.set_cursor(0, down_pos).print("B:").print(down);
		}
		disp.set_cursor(1, 0);
	} else if (button == Button::B && event == Event::CLICK) {
		// move down
		if (zone == last_zone) {
			return;
		}
		uint8_t prev_zone = zone++;
		show_zone();
		if (zone == last_zone) {
			// hide 'down'
			disp.set_cursor(0, down_pos).print("   ");
		} else if (prev_zone == first_zone) {
			// show 'up'
			disp.set_cursor(0, up_pos).print("A:").print(up);
		}
		disp.set_cursor(1, 0);
	} else if (button == Button::C && event == Event::CLICK) {
		// change activation
		uint8_t a = (uint8_t)activation[zone] + 1;
		if (a == 3) {
			a = 0;
		}
		activation[zone] = (Activation)a;
		show_activation();
		disp.set_cursor(1, 0);
	} else if (button == Button::D && event == Event::CLICK) {
		// exit
		activate_previous();
	}
}