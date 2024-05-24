//
// Created by independent-variable on 5/24/2024.
//

#include "./call_tasks.h"
#include "sim900_callbacks.h"
#include "./state.h"
#include "./service_tasks.h"
#include "./callback_handling.h"
#include <string.h>
namespace gsm {
	volatile sim900::CallState actual_call_state = sim900::CallState::ENDED;
	volatile sim900::CallState handled_call_state = sim900::CallState::ENDED;
	volatile sim900::CallDirection call_direction = sim900::CallDirection::INCOMING;
	char phone_number[MAX_PHONE_LENGTH + 1];
}

bool gsm::accept_call() {
	using namespace sim900;

	static constexpr auto accept_done = [](Result res) {
		bool call_accepted = false;
		if (res == Result::OK) {
			actual_call_state = CallState::SPEAKING;
			handled_call_state = CallState::SPEAKING;
			call_accepted = true;
		} else if (res == Result::ERROR) {
			if (call_direction == CallDirection::INCOMING && actual_call_state != CallState::SPEAKING) {
				// call definitely ended(was it ongoing or not)
				actual_call_state = CallState::ENDED;
				handled_call_state = CallState::ENDED;
			} // else - ERROR because of inappropriately usage
		} else {
			schedule(Task::REBOOT);
		}
		give_call_mutex();
		future_result({.call_accepted = call_accepted});
		execute_scheduled();
	};

	take_call_mutex();
	sim900::accept_call(accept_done);
	return future_result().call_accepted;
}

void gsm::end_call() {
	using namespace sim900;

	static constexpr auto end_done = [](Result res) {
		bool call_ended = false;
		if (res == Result::OK || res == Result::ERROR) {
			// result is OK event if there is no ongoing call
			actual_call_state = CallState::ENDED;
			handled_call_state = CallState::ENDED;
			call_ended = true;
		} else {
			schedule(Task::REBOOT);
		}
		give_call_mutex();
		future_result({.call_ended = call_ended});
		execute_scheduled();
	};

	take_call_mutex();
	sim900::end_call(end_done);
	(void)future_result().call_ended; // do not care about value
}

gsm::Dialing gsm::call(char * number) {
	using namespace sim900;

	static constexpr auto call_done = [](Result res) {
		if (res == Result::OK) {
			actual_call_state = CallState::RINGING;
			handled_call_state = CallState::RINGING;
			call_direction = CallDirection::OUTGOING;
			// future result is set in "call state changed" callback
		} else if (res == Result::ERROR) {
			// didn't establish a call
			// don't touch state - maybe a call was attempted during already ongoing call
			give_call_mutex();
			future_result({.dialing = Dialing::ERROR});
		} else {
			give_call_mutex();
			future_result({.dialing = Dialing::ERROR});
			schedule(Task::REBOOT);
		}
		execute_scheduled();
	};

	take_call_mutex();
	sim900::call(number, call_done);
	return future_result().dialing;
}

static void call_dialed(gsm::Dialing dialing) {
	using namespace gsm;
	using sim900::CallState;

	if (dialing == Dialing::DONE) {
		handled_call_state = CallState::SPEAKING;
	} else {
		handled_call_state = CallState::ENDED;
	}
	give_call_mutex();
	future_result({.dialing = dialing});
}

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
		// transition to ENDED is done in "call ended" callback
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
	if (call_direction == CallDirection::INCOMING) {
		if (one_of(actual_call_state, CallState::RINGING, CallState::SPEAKING)) {
			actual_call_state = CallState::ENDED;
			handle(Event::CALL_STATE_CHANGED);
			return;
		}
	} else { // OUTGOING
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

void gsm::terminate_calls() {
	using namespace sim900;
	if (actual_call_state != CallState::ENDED) {
		sim900::on_call_end(sim900::CallEnd::NORMAL);
	}
}