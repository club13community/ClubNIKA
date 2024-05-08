//
// Created by independent-variable on 4/5/2024.
//
#include "../controllers.h"
#include "../display.h"
#include "../symbols.h"

namespace user_interface {
	class ZoneViewer : public Controller {
	protected:
		void activate(bool init) override;
		void handle(keyboard::Button button, keyboard::Event event) override;
	};

	static ZoneViewer zone_viewer_obj;
	/** Shows activity of zones */
	Controller * const zone_viewer = &zone_viewer_obj;
}

using namespace user_interface;

void ZoneViewer::activate(bool init) {
	disp.clear()
			.define('\0', symbol::exit)
			.define('\1', symbol::ua_U);

	// show 'exit'
	disp.set_cursor(0, 0);
	disp << "D:" << '\0';
	// show zones
	disp.set_cursor(1, 0);
	disp << "AKT\1BHI:12345678";
}

void ZoneViewer::handle(keyboard::Button button, keyboard::Event event) {
	using keyboard::Button, keyboard::Event;
	if (button == Button::D && event == Event::CLICK) {
		yield();
	}
}