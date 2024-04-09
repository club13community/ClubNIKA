//
// Created by independent-variable on 4/5/2024.
//
#include "../controller.h"
#include "../display.h"
#include "../custom_chars.h"
#include "list.h"

namespace user_interface {
	class ZoneViewer : public Controller {
	public:
		void activate() override;
		void handle(keyboard::Button button, keyboard::Event event) override;
	};

	static ZoneViewer zone_viewer_obj;
	/** Shows activity of zones */
	Controller * const zone_viewer = &zone_viewer_obj;
}

using namespace user_interface;

void ZoneViewer::activate() {
	disp
	.put_out_on_inactivity()
	.light_up()
	.clear()
	.define('\0', symbol::exit)
	.define('\1', symbol::ua_U);

	// show 'exit'
	disp.cursor(0, 0);
	disp << "D:" << '\0';
	// show zones
	disp.cursor(1, 0);
	disp << "AKT\1BHI:12345678";
}

void ZoneViewer::handle(keyboard::Button button, keyboard::Event event) {
	using keyboard::Button, keyboard::Event;
	if (button == Button::D && event == Event::CLICK) {
		activate_previous();
	}
}