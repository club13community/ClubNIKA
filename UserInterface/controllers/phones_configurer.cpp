//
// Created by independent-variable on 4/5/2024.
//
#include "../display.h"
#include "../controller.h"
#include "../custom_chars.h"
#include "list.h"
#include <string.h>

#define MAX_PHONE_LENGTH	11U
#define MAX_PHONE_NUMBER	3U

namespace user_interface {
	class PhoneConfigurer : public Controller {
	private:
		char phones[9][MAX_PHONE_LENGTH + 1];
		uint8_t phones_num = 0;
		uint8_t phone_ind = 0;

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
			disp.cursor(1, 0);
		}

	public:
		void activate() override ;
		void handle(keyboard::Button button, keyboard::Event event) override;

		void phone_added(char * new_phone);
		void number_changed(uint8_t index, char * new_phone);
		void phone_deleted(uint8_t index);
		void phone_moved(uint8_t old_index, uint8_t new_index);
	};

	static PhoneConfigurer configurer;
	Controller * const phone_configurer = &configurer;

	class PhoneEditor : public Controller {
	private:
		char * phone;
		uint8_t index;
	public:
		void activate() override ;
		void handle(keyboard::Button button, keyboard::Event event) override;

		void prepare_to_edit(char * phone, uint8_t index);
		void number_changed(char * new_number);
		void priority_changed(uint8_t new_priority);
	};

	static PhoneEditor phone_editor;

	class NumberEditor : public Controller {
	private:
		char * phone;
		char phone_buf[MAX_PHONE_LENGTH + 1];
		uint8_t phone_len;
		bool edit;

		static constexpr char del = 0x7F, ok = '\1', cancel = '\2', cursor = 0x5F;
		static constexpr uint8_t del_pos = 0, ok_pos = 9, cancel_pos = 13;
	public:
		void prepare_to_add();
		void prepare_to_edit(char * number);
		void activate() override;
		void handle(keyboard::Button button, keyboard::Event event) override;
	};

	static NumberEditor number_editor;

	class PriorityEditor : public Controller {
	private:
		uint8_t priority;

		static constexpr uint8_t prio_pos = 13;
	public:
		void activate() override;
		void handle(keyboard::Button button, keyboard::Event event) override;

		void prepare_to_edit(uint8_t priority);
	};

	static PriorityEditor priority_editor;
}

using namespace user_interface;

void PhoneConfigurer::activate() {
	disp
	.put_out_on_inactivity()
	.light_up()
	.clear()
	.define(up, symbol::up)
	.define(down, symbol::down)
	.define(enter, symbol::enter)
	.define(exit, symbol::exit)
	.cursor(0, 0);
	// 1st row
	if (phone_ind > 0) {
		disp[0] << "A:" << up;
	}
	if (phone_ind < phones_num && phone_ind < MAX_PHONE_NUMBER - 1) {
		// if phone == phone_num -> show 'empty'
		disp[4] << "B:" << down;
	}
	disp [9] << "C:" << enter << " D:" << exit;
	// 2nd row
	disp.cursor(1, 0);
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
			disp.push_cursor().cursor(0, up_pos).print("   ").pop_cursor();
		}
		if (prev_ind == phones_num || prev_ind == MAX_PHONE_NUMBER - 1) {
			// show 'down'
			disp.push_cursor().cursor(0, down_pos).print("B:").print(down).pop_cursor();
		}
	} else if (button == Button::B && event == Event::CLICK) {
		// move down
		if (phone_ind == phones_num || phone_ind == MAX_PHONE_NUMBER - 1) {
			return;
		}
		uint8_t prev_ind = phone_ind++;
		print_phone();
		if (phone_ind == phones_num || phone_ind == MAX_PHONE_NUMBER - 1) {
			// hide 'down'
			disp.push_cursor().cursor(0, down_pos).print("   ").pop_cursor();
		}
		if (prev_ind == 0) {
			// show 'up'
			disp.push_cursor().cursor(0, up_pos).print("A:").print(up).pop_cursor();
		}
	} else if (button == Button::C && event == Event::CLICK) {
		// edit/add phone
		if (phone_ind == phones_num) {
			number_editor.prepare_to_add();
			activate_next(&number_editor);
		} else {
			phone_editor.prepare_to_edit(phones[phone_ind], phone_ind);
			activate_next(&phone_editor);
		}
	} else if (button == Button::D && event == Event::CLICK) {
		// exit
		activate_previous();
	}
}

void PhoneConfigurer::phone_added(char * new_phone) {
	strcpy(phones[phones_num++], new_phone);
}

void PhoneConfigurer::number_changed(uint8_t index, char * new_phone) {
	strcpy(phones[index], new_phone);
}

void PhoneConfigurer::phone_deleted(uint8_t index) {
	// shift phones up
	for (uint8_t dst = index, src = index + 1; src < phones_num; dst++, src++) {
		strcpy(phones[dst], phones[src]);
	}
	phones_num--;
	if (phone_ind > phones_num) {
		phone_ind = phones_num;
	}
}

void PhoneConfigurer::phone_moved(uint8_t old_index, uint8_t new_index) {
	if (new_index >= phones_num) {
		new_index = phones_num - 1;
	}
	// shift phones between old and new indexes
	char phone[MAX_PHONE_LENGTH];
	strcpy(phone, phones[old_index]);
	int8_t step = new_index < old_index ? -1 : 1;
	uint8_t dst = old_index, src = old_index + step;
	while (dst != new_index) {
		strcpy(phones[dst], phones[src]);
		dst += step;
		src += step;
	}
	// put phone in new position
	strcpy(phones[new_index], phone);
	// show this phone in new position
	phone_ind = new_index;
}

/* --- Phone editing --- */

void PhoneEditor::activate() {
	disp
	.put_out_on_inactivity()
	.light_up()
	.clear()
	.define('\1', symbol::ua_U)
	.define('\2', symbol::ua_D)
	.define('\3', symbol::ua_L)
	.define('\4', symbol::ua_P)
	.define('\5', symbol::exit)
	.cursor(0, 0).print("A:B\1\2A\3.  B:PE\2.")
	.cursor(1, 0).print("C:\4PIOPIT.   D:\5");
}

void PhoneEditor::handle(keyboard::Button button, keyboard::Event event) {
	using keyboard::Button, keyboard::Event;
	if (button == Button::A && event == Event::CLICK) {
		// delete
		configurer.phone_deleted(index);
		activate_previous();
	} else if (button == Button::B && event == Event::CLICK) {
		// change number
		number_editor.prepare_to_edit(phone);
		activate_next(&number_editor);
	} else if (button == Button::C && event == Event::CLICK) {
		// change priority
		priority_editor.prepare_to_edit(index + 1);
		activate_next(&priority_editor);
	} else if (button == Button::D && event == Event::CLICK) {
		activate_previous();
	}
}

void PhoneEditor::prepare_to_edit(char * phone, uint8_t index) {
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

void NumberEditor::prepare_to_edit(char * number) {
	strcpy(phone_buf, number);
	phone_len = strlen(number);
	edit = true;
}

void NumberEditor::activate() {
	disp.clear()
			.define(ok, symbol::enter)
			.define(cancel, symbol::exit)
			.cursor(0, 1);
	// 1st row
	if (phone_len != 0) {
		disp[ok_pos] << "C:" << ok;
		disp[del_pos] << "A:" << del;
	}
	disp[cancel_pos] << "D:" << cancel;
	// 2nd row
	disp.cursor(1, 0);
	if (edit) {
		disp << phone_buf;
	}
	if (phone_len < MAX_PHONE_LENGTH) {
		disp.print(cursor).move(-1);
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
			disp.print(cursor).move(-1);
		}
		if (phone_len == 1) {
			disp.push_cursor()
			.cursor(0, del_pos).print("A:").print(del) // show 'delete'
			.cursor(0, ok_pos).print("C:").print(ok) // show 'enter'
			.pop_cursor();
		}
	} else if (button == Button::A && event == Event::CLICK) {
		// delete
		if (phone_len == 0) {
			return;
		}
		phone_buf[--phone_len] = '\0';
		disp.print(' ').move(-2).print(cursor).move(-1);
		if (phone_len == 0) {
			disp.push_cursor()
			.cursor(0, del_pos).print("   ") // hide 'delete'
			.cursor(0, ok_pos).print("   ") // hide 'ok'
			.pop_cursor();
		}
	} else if (button == Button::C && event == Event::CLICK) {
		// ok
		if (phone_len == 0) {
			return;
		}
		if (edit) {
			phone_editor.number_changed(phone_buf);
		} else {
			configurer.phone_added(phone_buf);
		}
		activate_previous();
	} else if (button == Button::D && event == Event::CLICK) {
		// cancel
		activate_previous();
	}
}

/* --- Edit priority --- */

void PriorityEditor::activate() {
	disp
	.put_out_on_inactivity()
	.light_up()
	.clear()
	.define('\1', symbol::enter)
	.define('\2', symbol::exit)
	.define('\3', symbol::ua_P)
	.cursor(0, 0).print("  C:\1      D:\2  ")
	.cursor(1, 0).print("\3PIOPITET:  ")
	.cursor(1, prio_pos).print((int)priority)
	.cursor(1, prio_pos);
}

void PriorityEditor::handle(keyboard::Button button, keyboard::Event event) {
	using keyboard::Button, keyboard::Event;
	if (button >= Button::N1 && button <= (Button)MAX_PHONE_NUMBER && event == Event::CLICK) {
		// change priority
		priority = (uint8_t)button;
		disp.print((int)priority).cursor(1, prio_pos);
	} else if (button == Button::C && event == Event::CLICK) {
		// ok
		phone_editor.priority_changed(priority);
		activate_previous();
	} else if (button == Button::D && event == Event::CLICK) {
		// cancel
		activate_previous();
	}
}

void PriorityEditor::prepare_to_edit(uint8_t priority) {
	this->priority = priority;
}