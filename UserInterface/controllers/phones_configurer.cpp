//
// Created by independent-variable on 4/5/2024.
//
#include "../display.h"
#include "../controllers.h"
#include "../symbols.h"
#include <string.h>
#include "settings.h"

namespace user_interface {
	class PhoneConfigurer : public Controller {
	private:
		const char (*phones)[MAX_PHONE_LENGTH + 1];
		uint8_t phones_num;
		uint8_t phone_ind;

		const char up = '\0', down = '\1', enter = '\2', exit = '\3';
		static constexpr uint8_t up_pos = 0, down_pos = 4;

		/** Expects cursor on the 2nd row.*/
		inline void print_phone() {
			if (phone_ind < phones_num) {
				disp[0] << phone_ind + 1 << ") " << phones[phone_ind];
				uint8_t blank_len = 16 - 3 - strlen(phones[phone_ind]);
				while (blank_len-- > 0) {
					disp << ' ';
				}
			} else {
				disp.define('\4', symbol::ua_P).define('\5', symbol::ua_Y);
				disp[0] << phone_ind + 1 << ") " << "   \4\5CTO     ";
			}
			disp.set_cursor(1, 0);
		}

	protected:
		void activate(bool init) override;
		void handle(keyboard::Button button, keyboard::Event event) override;
	public:
		void phone_added(char * new_phone);
		void number_changed(uint8_t index, char * new_phone);
		void phone_deleted(uint8_t index);
		void phone_moved(uint8_t old_index, uint8_t new_index);
	};

	static PhoneConfigurer configurer;
	Controller * const phone_configurer = &configurer;

	class PhoneEditor : public Controller {
	private:
		const char * phone;
		uint8_t index;
	protected:
		void activate(bool init) override ;
		void handle(keyboard::Button button, keyboard::Event event) override;
	public:
		void prepare_to_edit(const char * phone, uint8_t index);
		void number_changed(char * new_number);
		void priority_changed(uint8_t new_priority);
	};

	static PhoneEditor phone_editor;

	class NumberEditor : public Controller {
	private:
		char phone_buf[MAX_PHONE_LENGTH + 1];
		uint8_t phone_len;
		bool edit;

		static constexpr char del = 0x7F, ok = '\1', cancel = '\2', cursor = 0x5F;
		static constexpr uint8_t del_pos = 0, ok_pos = 9, cancel_pos = 13;
	protected:
		void activate(bool init) override;
		void handle(keyboard::Button button, keyboard::Event event) override;
	public:
		void prepare_to_add();
		void prepare_to_edit(const char * number);

	};

	static NumberEditor number_editor;

	class PriorityEditor : public Controller {
	private:
		uint8_t priority;

		static constexpr uint8_t prio_pos = 13;
	protected:
		void activate(bool init) override;
		void handle(keyboard::Button button, keyboard::Event event) override;
	public:
		void prepare_to_edit(uint8_t priority);
	};

	static PriorityEditor priority_editor;
}

using namespace user_interface;

void PhoneConfigurer::activate(bool init) {
	if (init) {
		phones = get_phones();
		phones_num = get_phones_count();
		phone_ind = 0;
	}
	disp.clear()
			.define(up, symbol::up)
			.define(down, symbol::down)
			.define(enter, symbol::enter)
			.define(exit, symbol::exit)
			.set_cursor(0, 0);

	// 1st row
	if (phone_ind > 0) {
		disp[0] << "A:" << up;
	}
	if (phone_ind < phones_num && phone_ind < MAX_PHONE_NUMBERS - 1) {
		// if phone == phone_num -> show 'empty'
		disp[4] << "B:" << down;
	}
	disp [9] << "C:" << enter << " D:" << exit;
	// 2nd row
	disp.set_cursor(1, 0);
	print_phone();
}

void PhoneConfigurer::handle(keyboard::Button button, keyboard::Event event) {
	using keyboard::Button, keyboard::Event;
	if (button == Button::A && event == Event::CLICK) {
		// move up
		if (phone_ind == 0) {
			return;
		}
		uint8_t prev_ind = phone_ind--;
		print_phone();
		if (phone_ind == 0) {
			// hide 'up'
			disp.push_cursor().set_cursor(0, up_pos).print("   ").pop_cursor();
		}
		if (prev_ind == phones_num || prev_ind == MAX_PHONE_NUMBERS - 1) {
			// show 'down'
			disp.push_cursor().set_cursor(0, down_pos).print("B:").print(down).pop_cursor();
		}
	} else if (button == Button::B && event == Event::CLICK) {
		// move down
		if (phone_ind == phones_num || phone_ind == MAX_PHONE_NUMBERS - 1) {
			return;
		}
		uint8_t prev_ind = phone_ind++;
		print_phone();
		if (phone_ind == phones_num || phone_ind == MAX_PHONE_NUMBERS - 1) {
			// hide 'down'
			disp.push_cursor().set_cursor(0, down_pos).print("   ").pop_cursor();
		}
		if (prev_ind == 0) {
			// show 'up'
			disp.push_cursor().set_cursor(0, up_pos).print("A:").print(up).pop_cursor();
		}
	} else if (button == Button::C && event == Event::CLICK) {
		// edit/add phone
		if (phone_ind == phones_num) {
			number_editor.prepare_to_add();
			invoke(&number_editor);
		} else {
			phone_editor.prepare_to_edit(phones[phone_ind], phone_ind);
			invoke(&phone_editor);
		}
	} else if (button == Button::D && event == Event::CLICK) {
		// exit
		yield();
	}
}

void PhoneConfigurer::phone_added(char * new_phone) {
	add_phone(new_phone);
	phones_num = get_phones_count();
}

void PhoneConfigurer::number_changed(uint8_t index, char * new_phone) {
	set_phone(index, new_phone);
}

void PhoneConfigurer::phone_deleted(uint8_t index) {
	if (delete_phone(index)) {
		phones_num = get_phones_count();
		if (phone_ind > phones_num) {
			phone_ind = phones_num;
		}
	}
}

void PhoneConfigurer::phone_moved(uint8_t old_index, uint8_t new_index) {
	if (new_index >= phones_num) {
		new_index = phones_num - 1;
	}
	if (shift_phone(old_index, new_index)) {
		// show this phone in new position
		phone_ind = new_index;
	}
}

/* --- Phone editing --- */

void PhoneEditor::activate(bool init) {
	disp.clear()
			.define('\1', symbol::ua_U)
			.define('\2', symbol::ua_D)
			.define('\3', symbol::ua_L)
			.define('\4', symbol::ua_P)
			.define('\5', symbol::exit)
			.set_cursor(0, 0).print("A:B\1\2A\3.  B:PE\2.")
			.set_cursor(1, 0).print("C:\4PIOPIT.   D:\5");
}

void PhoneEditor::handle(keyboard::Button button, keyboard::Event event) {
	using keyboard::Button, keyboard::Event;
	if (button == Button::A && event == Event::CLICK) {
		// delete
		configurer.phone_deleted(index);
		yield();
	} else if (button == Button::B && event == Event::CLICK) {
		// change number
		number_editor.prepare_to_edit(phone);
		invoke(&number_editor);
	} else if (button == Button::C && event == Event::CLICK) {
		// change priority
		priority_editor.prepare_to_edit(index + 1);
		invoke(&priority_editor);
	} else if (button == Button::D && event == Event::CLICK) {
		yield();
	}
}

void PhoneEditor::prepare_to_edit(const char * phone, uint8_t index) {
	this->phone = phone;
	this->index = index;
}

void PhoneEditor::number_changed(char * new_number) {
	configurer.number_changed(index, new_number);
}

void PhoneEditor::priority_changed(uint8_t new_priority) {
	configurer.phone_moved(index, new_priority - 1);
}

/* --- Number editing --- */

void NumberEditor::prepare_to_add() {
	phone_buf[0] = '\0';
	phone_len = 0;
	edit = false;
}

void NumberEditor::prepare_to_edit(const char * number) {
	strcpy(phone_buf, number);
	phone_len = strlen(number);
	edit = true;
}

void NumberEditor::activate(bool init) {
	disp.clear()
			.define(ok, symbol::enter)
			.define(cancel, symbol::exit)
			.set_cursor(0, 1);
	// 1st row
	if (phone_len >= MIN_PHONE_LENGTH) {
		disp[ok_pos] << "C:" << ok;
	}
	if (phone_len != 0) {
		disp[del_pos] << "A:" << del;
	}
	disp[cancel_pos] << "D:" << cancel;
	// 2nd row
	disp.set_cursor(1, 0);
	if (edit) {
		disp << phone_buf;
	}
	if (phone_len < MAX_PHONE_LENGTH) {
		disp.print(cursor).move_cursor(-1);
	}
}

void NumberEditor::handle(keyboard::Button button, keyboard::Event event) {
	using keyboard::Button, keyboard::Event;
	if (button >= Button::N0 && button <= Button::N9 && event == Event::CLICK) {
		// enter digit
		if (phone_len >= MAX_PHONE_LENGTH) {
			return;
		}
		char digit = (0x30 | (uint8_t)button);
		phone_buf[phone_len++] = digit;
		phone_buf[phone_len] = '\0';

		disp << digit;
		if (phone_len < MAX_PHONE_LENGTH) {
			// possible to add digit - show cursor
			disp.print(cursor).move_cursor(-1);
		}
		if (phone_len == 1) {
			disp.push_cursor()
					.set_cursor(0, del_pos).print("A:").print(del) // show 'delete'
					.pop_cursor();
		}
		if (phone_len == MIN_PHONE_LENGTH) {
			disp.push_cursor()
					.set_cursor(0, ok_pos).print("C:").print(ok) // show 'enter'
					.pop_cursor();
		}
	} else if (button == Button::A && event == Event::CLICK) {
		// delete
		if (phone_len == 0) {
			return;
		}
		phone_buf[--phone_len] = '\0';
		disp.print(' ').move_cursor(-2).print(cursor).move_cursor(-1);
		if (phone_len == 0) {
			disp.push_cursor()
					.set_cursor(0, del_pos).print("   ") // hide 'delete'
					.pop_cursor();
		}
		if (phone_len == MIN_PHONE_LENGTH - 1) {
			disp.push_cursor()
					.set_cursor(0, ok_pos).print("   ") // hide 'ok'
					.pop_cursor();
		}
	} else if (button == Button::C && event == Event::CLICK) {
		// ok
		if (phone_len < MIN_PHONE_LENGTH) {
			return;
		}
		if (edit) {
			phone_editor.number_changed(phone_buf);
		} else {
			configurer.phone_added(phone_buf);
		}
		yield();
	} else if (button == Button::D && event == Event::CLICK) {
		// cancel
		yield();
	}
}

/* --- Edit priority --- */

void PriorityEditor::activate(bool init) {
	disp.clear()
			.define('\1', symbol::enter)
			.define('\2', symbol::exit)
			.define('\3', symbol::ua_P)
			.set_cursor(0, 0).print("  C:\1      D:\2  ")
			.set_cursor(1, 0).print("\3PIOPITET:  ")
			.set_cursor(1, prio_pos).print((int) priority)
			.set_cursor(1, prio_pos);
}

void PriorityEditor::handle(keyboard::Button button, keyboard::Event event) {
	using keyboard::Button, keyboard::Event;
	if (button >= Button::N1 && button <= (Button)MAX_PHONE_NUMBERS && event == Event::CLICK) {
		// change priority
		priority = (uint8_t)button;
		disp.print((int) priority).set_cursor(1, prio_pos);
	} else if (button == Button::C && event == Event::CLICK) {
		// ok
		phone_editor.priority_changed(priority);
		yield();
	} else if (button == Button::D && event == Event::CLICK) {
		// cancel
		yield();
	}
}

void PriorityEditor::prepare_to_edit(uint8_t priority) {
	this->priority = priority;
}