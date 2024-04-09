//
// Created by independent-variable on 4/5/2024.
//

#pragma once
#include "keyboard.h"

namespace user_interface {
	class Controller {
	private:
		Controller * previous;
	protected:
		/** Invoked by controller, when it decides to give away control to previous */
		void activate_previous();
		/** Invoked by controller, when it decides to give away control to next.
		 * Control is returned to this after next finishes */
		void activate_next(Controller * next);
		/** Tracking of long inactivity is used to automatically turn off display's backlight.
		 * This method is invoked by controlled to notify that some activity(except pushing buttons) is going on.
		 * Again, keyboard activity is already tracked - no reason to invoke this on button events.
		 * @returns true if UI before invocation was active */
		bool get_and_report_activity();
	public:
		/** Invoked when particular controller should control LCD display and react on keyboard events. */
		virtual void activate() = 0;
		virtual void handle(keyboard::Button button, keyboard::Event event) = 0;
		/** Invoked when user does not interact with UI and no other activity is reported for a long time */
		virtual void on_ui_inactive() {}
	};
}