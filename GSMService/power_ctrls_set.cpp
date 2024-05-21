//
// Created by independent-variable on 5/14/2024.
//
#include "./CtrlsSet.h"
#include "sim900.h"

/*static void power_on_done(bool success) {
	constexpr void (* reboot)() = []() {
		sim900::turn_on(power_on_done);
	};
	if (success) {
		gsm::end_handler();
	} else {
		sim900::turn_off(reboot);
	}
}

static void power_on(gsm::Handler handler) {
	gsm::set_end(handler);
	sim900::turn_on(power_on_done);
}

static void power_off_done() {
	gsm::end_handler();
}

static void power_off(gsm::Handler handler) {
	set_end(handler);
	sim900::turn_off(power_off_done);
}

void gsm::CtrlsSet::power_on(Handler handler) {

}

void gsm::CtrlsSet::power_off(Handler handler) {

}*/