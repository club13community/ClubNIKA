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
	class AlarmDisabler : public Controller {
	private:
		static constexpr char del = 0x7F;
		static constexpr uint8_t del_pos = 13;

		char passw[PASSWORD_LENGTH + 1];
		uint8_t passw_len;
	public:
		void activate(bool init) override;
		void handle(keyboard::Button button, keyboard::Event event) override;
	};

	static AlarmDisabler disabler;
	Controller * const alarm_disabler = &disabler;

	class WrongPasswordNotifier : public Controller {
	protected:
		void activate(bool init) override;
		void handle(keyboard::Button button, keyboard::Event event) override;
	};

	static WrongPasswordNotifier notifier;
}

using namespace user_interface;

void AlarmDisabler::activate(bool init) {
	if (init) {
		passw[0] = '\0';
		passw_len = 0;
	}
	disp.clear()
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
			alarm::disarm();
			user_interface::alarm_disarmed(); // this will change controller
		} else {
			invoke(&notifier);
		}
	}
}

/* --- wrong password notifier --- */

void WrongPasswordNotifier::activate(bool init) {
	disp.clear()
			.define('\1', symbol::ua_P).define('\2', symbol::ua_L).define('\3', symbol::ua_MILD)
			.set_cursor(0, 1).print("HE\1PAB. \1APO\2\3")
			.set_cursor(1, 6).print("C:OK");
}

void WrongPasswordNotifier::handle(keyboard::Button button, keyboard::Event event) {
	using keyboard::Button, keyboard::Event;
	if (button == Button::C && event == Event::CLICK) {
		yield();
	}
}