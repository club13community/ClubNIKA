//
// Created by independent-variable on 4/8/2024.
//
#include "../display.h"
#include "../controllers.h"
#include "../symbols.h"

static uint16_t old_delay = 120;

namespace user_interface {
	class DelayEditor : public Controller {
	private:
		static constexpr char ok = '\0', cancel = '\1', v = '\2';
		static constexpr uint8_t inc_pos = 0, dec_pos = 4, delay_pos = 7;
		static constexpr uint16_t min_delay_s = 20, max_delay_s = 15 * 60;
		static constexpr uint8_t step_s = 20;

		uint16_t delay_s = 120;
		bool inc_active, dec_active;

	private:
		void print_delay();
		void update_delay();

		inline bool may_inc() {
			return delay_s + step_s <= max_delay_s;
		}

		inline bool may_dec() {
			return delay_s >= step_s && delay_s - step_s >= min_delay_s;
		}

		/** @param num 0..99
 		* @returns length of text */
		inline uint8_t print(uint8_t num) {
			if (num > 9) {
				disp.print(char(0x30 | num / 10)).print(char(0x30 | num % 10));
				return 2;
			} else {
				disp.print(char(0x30U | num));
				return 1;
			}
		}
	public:
		void activate() override;
		void handle(keyboard::Button button, keyboard::Event event) override;
	};

	static DelayEditor editor;
	Controller * const delay_editor = &editor;
}

using namespace user_interface;

void DelayEditor::activate() {
	disp
	.put_out_on_inactivity()
	.light_up()
	.clear()
	.define(ok, symbol::enter).define(cancel, symbol::exit).define(v, symbol::ua_v);
	delay_s = old_delay; // todo: init on first entrance
	// 1st line
	inc_active = may_inc();
	if (inc_active) {
		disp.set_cursor(0, inc_pos).print("A:+");
	}
	dec_active = may_dec();
	if (dec_active) {
		disp.set_cursor(0, dec_pos).print("B:-");
	}
	disp
			.set_cursor(0, 9).print("C:").print(ok)
			.set_cursor(0, 13).print("D:").print(cancel);
	// 2nd line
	disp.set_cursor(1, 0).print("3ATP.:");
	print_delay();
}

void DelayEditor::handle(keyboard::Button button, keyboard::Event event) {
	using keyboard::Button, keyboard::Event;
	if (button == Button::A && event == Event::CLICK) {
		// inc
		if (!may_inc()) {
			return;
		}
		delay_s += step_s;
		print_delay();
		if (!may_inc()) {
			// hide inc
			disp.push_cursor().set_cursor(0, inc_pos).print("   ").pop_cursor();
			inc_active = false;
		}
		if (!dec_active) {
			// show dec
			disp.push_cursor().set_cursor(0, dec_pos).print("B:-").pop_cursor();
			dec_active = true;
		}
	} else if (button == Button::B && event == Event::CLICK) {
		// dec
		if (!may_dec()) {
			return;
		}
		delay_s -= step_s;
		print_delay();
		if (!may_dec()) {
			// hide dec
			disp.push_cursor().set_cursor(0, dec_pos).print("   ").pop_cursor();
			dec_active = false;
		}
		if (!inc_active) {
			// show inc
			disp.push_cursor().set_cursor(0, inc_pos).print("A:+").pop_cursor();
			inc_active = true;
		}
	} else if (button == Button::C && event == Event::CLICK) {
		// ok
		update_delay();
		activate_previous();
	} else if (button == Button::D && event == Event::CLICK) {
		// cancel
		activate_previous();
	}
}

void DelayEditor::print_delay() {
	uint8_t min = delay_s / 60, sec = delay_s % 60;
	uint8_t blanks = 8;
	disp.set_cursor(1, delay_pos);
	if (min > 0) {
		blanks -= print(min) - 3;
		disp << "x" << v << ' ';
	}
	if (sec > 0) {
		blanks -= print(sec) + 1;
		disp.print('c');
	}
	while (blanks-- > 0) {
		disp.print(' ');
	}
}

void DelayEditor::update_delay() {
	old_delay = delay_s;
}