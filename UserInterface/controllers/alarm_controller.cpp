//
// Created by independent-variable on 4/5/2024.
//
#include "../controllers.h"
#include "../display.h"
#include "../symbols.h"
#include "settings.h"

namespace user_interface {
	class AlarmController : public Controller {
	private:
		enum State {WAIT_ARMING, WAIT_DISARMING, NOTIFY_WRONG_PASSWORD};
		State state;
	protected:
		void activate(bool init) override;
		void handle(keyboard::Button button, keyboard::Event event) override;
	};

	static AlarmController controller;
	Controller * const alarm_controller = &controller;

	class AlarmEnabler {
	private:
		static constexpr uint8_t timer_pos = 9;
		uint16_t delay = 10;
	private:
		void print_delay();
	public:
		void activate();
		void handle(keyboard::Button button, keyboard::Event event);
		void tick();
	};

	static AlarmEnabler enabler;

	class AlarmDisabler {
	private:
		static constexpr char del = 0x7F;
		static constexpr uint8_t del_pos = 13;

		char passw[PASSWORD_LENGTH + 1] = "";
		uint8_t passw_len = 0;
	public:
		void activate();
		void handle(keyboard::Button button, keyboard::Event event);
	};

	static AlarmDisabler disabler;

	class WrongPasswordNotifier {
	public:
		void activate();
		void handle(keyboard::Button button, keyboard::Event event);
	};

	static WrongPasswordNotifier notifier;
}

using namespace user_interface;

void AlarmController::activate(bool init) {
	if (init) {
		state = WAIT_ARMING;
		handle_ui_inactivity(false);
	}
	switch (state) {
		case WAIT_ARMING:
			enabler.activate();
			break;
		case WAIT_DISARMING:
			disabler.activate();
			break;
		case NOTIFY_WRONG_PASSWORD:
			notifier.activate();
			break;
	}
}

void AlarmController::handle(keyboard::Button button, keyboard::Event event) {
	switch (state) {
		case WAIT_ARMING:
			enabler.handle(button, event);
			break;
		case WAIT_DISARMING:
			disabler.handle(button, event);
			break;
		case NOTIFY_WRONG_PASSWORD:
			notifier.handle(button, event);
			break;
	}
}

/* --- enabling alarm --- */

void AlarmEnabler::activate() {
	delay = 10; // todo load from config

	disp
			.light_up()
			.clear()
			.define('\1', symbol::ua_D).define('\2', symbol::ua_U)
			.set_cursor(0, 0).print("\1O AKT\2B.")
			.set_cursor(1, 3).print("A:BI\1MIHA");
	print_delay();
}

void AlarmEnabler::handle(keyboard::Button button, keyboard::Event event) {
	using keyboard::Button, keyboard::Event;
	if (button == Button::A && event == Event::CLICK) {
		// cancel
		// todo canceled
	}
}

void AlarmEnabler::tick() {
	delay -= 1;
	if (delay == 0) {
		// todo timeout elapsed
	} else {
		print_delay();
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

/* --- disabling alarm --- */

void AlarmDisabler::activate() {
	disp
	.light_up()
	.clear()
	.define('\1', symbol::ua_U).define('\2', symbol::ua_Y).define('\3', symbol::ua_P)
	.define('\4', symbol::ua_L).define('\5', symbol::ua_MILD);
	// 1st row
	if (passw_len == PASSWORD_LENGTH) {
		disp.set_cursor(0, 0).print("   C:B\1MKH\2T\1   ");
	} else {
		disp.set_cursor(0, 0).print("   AKT\1BOBAHA   ");
	}
	// 2nd row
	disp.set_cursor(1, 0).print("\3APO\4\5:");
	if (passw_len > 0) {
		disp.set_cursor(1, del_pos).print("A:").print(del);
	}
	disp.set_cursor(1, 8).print(passw);
}

void AlarmDisabler::handle(keyboard::Button button, keyboard::Event event) {
	using keyboard::Button, keyboard::Event;
	if (button >= Button::N0 && button <= Button::N9 && event == Event::CLICK) {
		// enter digit
		if (passw_len >= PASSWORD_LENGTH) {
			return;
		}
		char dig = 0x30 | (uint8_t)button;
		passw[passw_len++] = dig;
		disp.print(dig);
		if (passw_len == 1) {
			// show 'del'
			disp.push_cursor().set_cursor(1, del_pos).print("A:").print(del).pop_cursor();
		}
		if (passw_len == PASSWORD_LENGTH) {
			// show 'enter'
			disp.push_cursor().set_cursor(0, 0).print("   C:B\1MKH\2T\1   ").pop_cursor();
		}
	} else if (button == Button::A && event == Event::CLICK) {
		// delete digit
		if (passw_len == 0) {
			return;
		}
		passw[--passw_len] = '\0';
		disp.move_cursor(-1).print(' ').move_cursor(-1);
		if (passw_len == 0) {
			// hide 'del'
			disp.push_cursor().set_cursor(1, del_pos).print("   ").pop_cursor();
		}
		if (passw_len == PASSWORD_LENGTH - 1) {
			// hide 'enter'
			disp.push_cursor().set_cursor(0, 0).print("   AKT\1BOBAHA   ").pop_cursor();
		}
	} else if (button == Button::C && event == Event::CLICK) {
		// enter password
		if (passw_len != PASSWORD_LENGTH) {
			return;
		}
		if (is_correct_password(passw)) {
			// todo
		}
		// todo
	}
}

/* --- wrong password notifier --- */

void WrongPasswordNotifier::activate() {
	disp
			.clear()
			.define('\1', symbol::ua_P).define('\2', symbol::ua_L).define('\3', symbol::ua_MILD)
			.set_cursor(0, 1).print("HE\1PAB. \1APO\2\3")
			.set_cursor(1, 6).print("C:OK");
}

void WrongPasswordNotifier::handle(keyboard::Button button, keyboard::Event event) {
	using keyboard::Button, keyboard::Event;
	if (button == Button::C && event == Event::CLICK) {
		// todo user notified
	}
}