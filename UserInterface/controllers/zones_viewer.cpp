//
// Created by independent-variable on 4/5/2024.
//
#include "../controllers.h"
#include "../display.h"
#include "../symbols.h"
#include "wired_zones.h"

/** How often to refresh zones */
#define REFRESH_PERIOD_ms	1000

namespace user_interface {
	class ZoneViewer : public Controller {
	private:
		uint8_t zones;

		/** Expects cursor at the position of the first zone */
		inline void print_zones() {
			disp.push_cursor();
			for (uint8_t i = 1, mask = 1; i <= 8; i++, mask <<= 1) {
				if (zones & mask) {
					disp.print(char(0x30U | i));
				} else {
					disp.print(' ');
				}
			}
			disp.pop_cursor();
		}
	protected:
		void activate(bool init) override;
		void handle(keyboard::Button button, keyboard::Event event) override;
		void delay_elapsed() override;
	};

	static ZoneViewer zone_viewer_obj;
	/** Shows activity of zones */
	Controller * const zone_viewer = &zone_viewer_obj;
}

using namespace user_interface;

void ZoneViewer::activate(bool init) {
	if (init) {
		zones = wired_zones::get_active();
		start_delay(REFRESH_PERIOD_ms);
	}
	disp.clear()
			.define('\0', symbol::exit)
			.define('\1', symbol::ua_U);

	// show 'exit'
	disp.set_cursor(0, 0);
	disp << "D:" << '\0';
	// show zones
	disp.set_cursor(1, 0);
	disp << "AKT\1BHI:";
	print_zones();
}

void ZoneViewer::handle(keyboard::Button button, keyboard::Event event) {
	using keyboard::Button, keyboard::Event;
	if (button == Button::D && event == Event::CLICK) {
		yield();
	}
}

void ZoneViewer::delay_elapsed() {
	uint8_t new_zones = wired_zones::get_active();
	if (new_zones != zones) {
		zones = new_zones;
		print_zones();
	}
	start_delay(REFRESH_PERIOD_ms);
}