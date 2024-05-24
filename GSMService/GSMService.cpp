#include "GSMService.h"
#include "sim900.h"
#include "sim900_callbacks.h"
#include "rtc.h"
#include "./state.h"
#include "./tasks.h"
#include "./event_handling.h"
#include <string.h>

void gsm::init_periph() {
	sim900::init_periph();
}

void gsm::start() {
	init_state();
	init_event_handling();
	init_task_execution();

	sim900::start();
}

uint8_t gsm::get_signal_strength() {
	using namespace sim900;
	return card_status == CardStatus::READY && registration == Registration::DONE ? signal_strength : 0;
}

void gsm::set_on_incoming_call(void (* callback)(char *)) {
	on_incoming_call = callback;
}

void gsm::set_on_call_ended(void (* callback)()) {
	on_call_ended = callback;
}

/*gsm::Controls & gsm::get_ctrl() {
	while (xSemaphoreTake(ctrl_mutex, portMAX_DELAY) == pdFALSE);
	//return CtrlsSet::inst;
}*/

static void call_dialed(gsm::Dialing dialing);

void sim900::on_call_update(CallState state, CallDirection direction, char * number) {
	using namespace gsm;
	if (direction == CallDirection::INCOMING) {
		if (state == CallState::RINGING && handled_call_state == CallState::ENDED) {
			// start new incoming if previous is already handled
			actual_call_state = CallState::RINGING;
			call_direction = direction;
			strcpy(phone_number, number);
			handle(Event::CALL_STATE_CHANGED);
			return;
		}
		// transition to SPEAKING is done inside "accept call" method
		if (state == CallState::ENDED && one_of(actual_call_state, CallState::RINGING, CallState::SPEAKING)) {
			actual_call_state = CallState::ENDED;
			handle(Event::CALL_STATE_CHANGED);
			return;
		}
	} else { // OUTGOING
		// transition to RINGING is done in "start call" method
		if (state == CallState::SPEAKING && gsm::actual_call_state == CallState::RINGING) {
			gsm::actual_call_state = CallState::SPEAKING;
			call_dialed(gsm::Dialing::DONE);
			return;
		}
		// transition to ENDED is done in "call ended" callback
	}
}

void sim900::on_call_end(CallEnd end) {
	using namespace gsm;
	if (call_direction == CallDirection::OUTGOING) {
		if (actual_call_state == CallState::RINGING) {
			actual_call_state = CallState::ENDED;
			call_dialed(end == CallEnd::NO_ANSWER || end == CallEnd::NORMAL ? Dialing::NO_ANSWER : Dialing::REJECTED);
			return;
		}
		if (actual_call_state == CallState::SPEAKING) {
			actual_call_state = CallState::ENDED;
			handle(Event::CALL_STATE_CHANGED);
			return;
		}
	}
}

void sim900::on_timestamp(rtc::Timestamp & timestamp) {

}

void end_call() {
	using namespace sim900;

	static constexpr auto end_done = [](Result res) {
		bool call_ended = false;
		if (res == Result::NO_RESPONSE || res == Result::CORRUPTED_RESPONSE) {
			gsm::handle(gsm::Event::ERROR);
		} else {
			// result is OK event if there is no ongoing call
			gsm::actual_call_state = CallState::ENDED;
			gsm::handled_call_state = CallState::ENDED;
			call_ended = true;
		}
		gsm::give_call_mutex();
		gsm::future_result({.call_ended = call_ended});
		gsm::execute_scheduled();
	};

	gsm::take_call_mutex();
	end_call(end_done);
	(void)gsm::future_result().call_ended; // do not care about value
}

bool accept_call() {
	using namespace sim900;

	static constexpr auto accept_done = [](Result res) {
		bool call_accepted = false;
		if (res == Result::OK) {
			gsm::actual_call_state = CallState::SPEAKING;
			gsm::handled_call_state = CallState::SPEAKING;
			call_accepted = true;
		} else if (res == Result::ERROR) {
			if (gsm::call_direction == CallDirection::INCOMING && gsm::actual_call_state != CallState::SPEAKING) {
				// call definitely ended(was it ongoing or not)
				gsm::actual_call_state = CallState::ENDED;
				gsm::handled_call_state = CallState::ENDED;
			} // else - ERROR because of inappropriately usage
		} else {
			gsm::handle(gsm::Event::ERROR);
		}
		gsm::give_call_mutex();
		gsm::future_result({.call_accepted = call_accepted});
		gsm::execute_scheduled();
	};

	gsm::take_call_mutex();
	accept_call(accept_done);
	return gsm::future_result().call_accepted;
}

gsm::Dialing call(char * number) {
	using namespace sim900;

	static constexpr auto call_done = [](Result res) {
		if (res == Result::OK) {
			gsm::actual_call_state = CallState::RINGING;
			gsm::handled_call_state = CallState::RINGING;
			gsm::call_direction = CallDirection::OUTGOING;
			// future result is set in "call state changed" callback
		} else if (res == Result::ERROR) {
			// didn't establish a call
			// don't touch state - maybe a call was attempted during already ongoing call
			gsm::give_call_mutex();
			gsm::future_result({.dialing = gsm::Dialing::ERROR});
		} else {
			gsm::give_call_mutex();
			gsm::future_result({.dialing = gsm::Dialing::ERROR});
			gsm::handle(gsm::Event::ERROR);
		}
		gsm::execute_scheduled();
	};

	gsm::take_call_mutex();
	call(number, call_done);
	return gsm::future_result().dialing;
}

static void call_dialed(gsm::Dialing dialing) {
	if (dialing == gsm::Dialing::DONE) {
		gsm::handled_call_state = sim900::CallState::SPEAKING;
	} else {
		gsm::handled_call_state = sim900::CallState::ENDED;
	}
	gsm::give_call_mutex();
	gsm::future_result({.dialing = dialing});
}


