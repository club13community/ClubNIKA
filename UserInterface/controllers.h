//
// Created by independent-variable on 4/5/2024.
//

#pragma once
#include "keyboard.h"

namespace user_interface {
	class Controller {
	private:
		Controller * previous;
		bool run_activity_timer;
		/** If true - keyboard is not blocked, displays is lighted up (you may put it out manually,
		 * but setting ui_active = true lights it up again). Set true when controller obtains control and UI activates.
		 * Set false only on activity timeout(if enabled). */
		bool ui_active;
		/** If > 0 - delay is ticking. */
		uint16_t delay_ms;
	public:
		void invoke();
		void resume();

		void on_keyboard(keyboard::Button button, keyboard::Event event) {
			if (ui_active) {
				// handle button
				handle(button, event);
			}
			// make UI active
			on_ui_activity();
		}

		void on_activity_timer();

		void on_delay_timer() {
			delay_elapsed();
		}

		/** Invoked by controller(or other who acts on controller's behalf),
		 * when it decides to give away control to previous */
		void yield();
		/** Invoked by controller(or other who acts on controller's behalf), when it decides to give away control to next.
		 * Control is returned to this after next yields */
		void invoke(Controller * next);
	private:
		/** Invoke this in handler of anything which is assumed UI activity */
		void on_ui_activity();
	protected:
		/** Invoked by controller to enable/disable default UI inactivity handling(put out backlight, block keyboard). */
		void handle_ui_inactivity(bool do_handle);
		void start_delay(uint16_t delay_ms);
		void cancel_delay();
		/** Invoked when particular controller should control LCD display and react on keyboard events.
		 * Display is already lighted up.
		 * @param init true when entering for the 1st time, false - return control from prev. */
		virtual void activate(bool init) = 0;
		virtual void handle(keyboard::Button button, keyboard::Event event) = 0;
		virtual void delay_elapsed() {}
	};

	extern Controller * const desktop;
	extern Controller * const menu;
	extern Controller * const zone_viewer;
	extern Controller * const zone_configurer;
	extern Controller * const phone_configurer;
	extern Controller * const password_editor;
	extern Controller * const delay_editor;
	extern Controller * const alarm_controller;
	extern Controller * const alarm_enabler;
	extern Controller * const alarm_disabler;
}