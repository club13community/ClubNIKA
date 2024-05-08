//
// Created by independent-variable on 4/8/2024.
//
#include "../symbols.h"
#include "../controllers.h"
#include "../display.h"
#include <string.h>
#include "settings.h"

namespace user_interface {
	class PasswordEditor : public Controller {
	private:
		enum Step {ENTER_OLD, REPORT_WRONG_OLD, ENTER_NEW, REPORT_SUCCESS, REPORT_HW_ERROR};
		static constexpr char del = 0x7F, enter = '\0', cancel = '\1';
		static constexpr uint8_t del_pos = 0, enter_pos = 9, cancel_pos = 13, passw_pos = 8;

		Step step;
		char passw[PASSWORD_LENGTH + 1], old_passw[PASSWORD_LENGTH + 1];
		uint8_t len;
	private:
		void init_state();
		void activate_step();
	public:
		PasswordEditor() {
			init_state();
		}
		void activate();
		void handle(keyboard::Button button, keyboard::Event event);
	};

	static PasswordEditor editor;
	Controller * const password_editor = &editor;
}

using namespace user_interface;

/** Sets initial state */
void PasswordEditor::init_state() {
	passw[0] = '\0';
	len = 0;
	step = ENTER_OLD;
}

void PasswordEditor::activate_step() {
	disp.clear();
	if (step == ENTER_OLD || step == ENTER_NEW) {
		// full show menu on 1st line
		if (len > 0) {
			disp.set_cursor(0, del_pos).print("A:").print(del);
		}
		if (len == PASSWORD_LENGTH) {
			disp.set_cursor(0, enter_pos).print("C:").print(enter);
		}
		disp.set_cursor(0, cancel_pos).print("D:").print(cancel);
		// 2 common chars for 2nd line
		disp.define('\2', symbol::ua_U).define('\3', symbol::ua_J).set_cursor(1, 0);
		// 2nd line
		if (step == ENTER_OLD) {
			disp.print("CTAP\2\3: ");
		} else {
			disp.print(" HOB\2\3: ");
		}
		disp.set_cursor(1, passw_pos).print(passw);
	} else {
		// common chars
		disp.define('\2', symbol::ua_P).define('\3', symbol::ua_L)
			.define('\4', symbol::ua_MILD).define('\5', symbol::ua_U);
		if (step == REPORT_WRONG_OLD) {
			disp.set_cursor(0, 0).print("HE\2PAB. \2APO\3\4");
		} else if (step == REPORT_HW_ERROR) {
			disp.set_cursor(0, 1).print("\2OM\5\3KA FLASH");
		} else {
			disp.set_cursor(0, 1).print("\2APO\3\4 3MIHEHO");
		}
		disp.set_cursor(1, 6).print("C:OK");
	}
}

void PasswordEditor::activate() {
	disp
	.put_out_on_inactivity()
	.light_up()
	.define(enter, symbol::enter).define(cancel, symbol::exit);
	activate_step();
}

void PasswordEditor::handle(keyboard::Button button, keyboard::Event event) {
	using keyboard::Button, keyboard::Event;
	if (button >= Button::N0 && button <= Button::N9 && event == Event::CLICK) {
		// entering digit
		if (step != ENTER_OLD && step != ENTER_NEW) {
			return;
		}
		if (len >= PASSWORD_LENGTH) {
			return;
		}
		char digit = 0x30 | (uint8_t)button;
		passw[len++] = digit;
		passw[len] = '\0';
		disp.print(digit);
		if (len == 1) {
			// show 'delete'
			disp.push_cursor().set_cursor(0, del_pos).print("A:").print(del).pop_cursor();
		}
		if (len == PASSWORD_LENGTH) {
			// show 'enter'
			disp.push_cursor().set_cursor(0, enter_pos).print("C:").print(enter).pop_cursor();
		}
	} else if (button == Button::A && event == Event::CLICK) {
		// delete
		if (step != ENTER_OLD && step != ENTER_NEW) {
			return;
		}
		if (len == 0) {
			return;
		}
		passw[--len] = '\0';
		disp.move_cursor(-1).print(' ').move_cursor(-1);
		if (len == 0) {
			// hide 'delete'
			disp.push_cursor().set_cursor(0, del_pos).print("   ").pop_cursor();
		}
		if (len == PASSWORD_LENGTH - 1) {
			// hide 'enter'
			disp.push_cursor().set_cursor(0, enter_pos).print("   ").pop_cursor();
		}
	} else if (button == Button::C && event == Event::CLICK) {
		// enter
		if (step == ENTER_OLD) {
			if (len != PASSWORD_LENGTH) {
				return;
			}
			// remember old passw., prepare to enter new password
			strcpy(old_passw, passw);
			passw[0] = '\0';
			len = 0;
			step = ENTER_NEW;
			activate_step();
		} else if (step == REPORT_WRONG_OLD || step == REPORT_HW_ERROR) {
			// return to entering old
			strcpy(passw, old_passw);
			step = ENTER_OLD;
			activate_step();
		} else if (step == ENTER_NEW) {
			if (len != PASSWORD_LENGTH) {
				return;
			}
			PasswordUpdate res = update_password(old_passw, passw);
			if (res == PasswordUpdate::DONE) {
				step = REPORT_SUCCESS;
				activate_step();
			} else if (res == PasswordUpdate::WRONG_PASSWORD) {
				step = REPORT_WRONG_OLD;
				activate_step();
			} else {
				// failed to persist
				step = REPORT_HW_ERROR;
				activate_step();
			}
		} else {
			// reporting success
			init_state();
			activate_previous();
		}
	} else if (button == Button::D && event == Event::CLICK) {
		// cancel
		if (step != ENTER_OLD && step != ENTER_NEW) {
			return; // button is not shown
		}
		init_state();
		activate_previous();
	}
}