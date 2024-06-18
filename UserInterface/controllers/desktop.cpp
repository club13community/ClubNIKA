//
// Created by independent-variable on 4/5/2024.
//
#include "../controllers.h"
#include "../display.h"
#include "../symbols.h"
#include "GSMService.h"
#include "SupplySystem.h"

#define STATUS_REFRESH_TIME_s	1U

namespace user_interface {
	class Desktop : public Controller {
	private:
		uint8_t gsm_signal;
		supply::Source source;
		uint8_t battery_pct;
	protected:
		void activate(bool init) override;
		void handle(keyboard::Button button, keyboard::Event event) override;
		void delay_elapsed() override;
	};

	static Desktop desktop_inst;
	Controller * const desktop = &desktop_inst;
}

using namespace user_interface;

static const char battery = '\0', signal = '\1', card = '\2', U = '\3', socket = '\4';
// place and max. length of "{GSM icon}100%"
static constexpr uint8_t gsm_pos = 0, gsm_len = 5;
// place and max. length of "{battery icon}100%" or "{socket icon}230V"
static constexpr uint8_t src_pos = 7, src_len = 5;

void Desktop::activate(bool init) {
	using supply::Source;
	if (init) {
		gsm_signal = gsm::get_signal_strength();
		if (gsm_signal == 0) {
			handle_ui_inactivity(false); // to keep display lighted up while no GSM signal
		}

		source = supply::get_source();
		if (source == Source::BATTERY) {
			battery_pct = supply::get_battery_pct();
		}

		start_delay(STATUS_REFRESH_TIME_s * 1000U);
	}

	disp.clear()
			.define(battery, symbol::battery)
			.define(signal, symbol::signal_level)
			.define(card, symbol::card)
			.define(U, symbol::ua_U)
			.define(socket, symbol::socket)
			.set_cursor(0, 0);

	disp.set_cursor(0, src_pos);
	if (source == Source::BATTERY) {
		disp.print(battery).print((int)battery_pct).print('%');
	} else {
		disp.print(socket).print("230V");
	}
	disp.set_cursor(0, gsm_pos).print(signal).print((int)gsm_signal).print('%');

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
	// update GSM signal strength
	uint8_t new_signal = gsm::get_signal_strength();
	if (new_signal != gsm_signal) {
		disp.set_cursor(0, gsm_pos + 1U).clear(gsm_len - 1).print((int)new_signal).print('%');
	}
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

	// update supply source status
	using supply::Source;
	Source new_source = supply::get_source();
	if (new_source == Source::BATTERY) {
		uint8_t new_bat_pct = supply::get_battery_pct();
		if (source != Source::BATTERY) {
			// refresh all
			disp.set_cursor(0, src_pos).clear(src_len)
				.print(battery).print((int)new_bat_pct).print('%');
		} else if (battery_pct != new_bat_pct) {
			// refresh only charge level
			disp.set_cursor(0, src_pos + 1U).clear(src_len - 1)
				.print((int)new_bat_pct).print('%');
		}
		battery_pct = new_bat_pct;
	} else if (source != Source::SOCKET) {
		disp.set_cursor(0, src_pos).clear(src_len).print(socket).print("230V");
	}
	source = new_source;

	start_delay(STATUS_REFRESH_TIME_s * 1000U);
}
