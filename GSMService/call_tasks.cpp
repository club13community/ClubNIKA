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
		future_result({.call_ended = call_ended});
		execute_scheduled();
	};

	// never breaks sequence of "call dialed" -> "call ended" callbacks
	sim900::end_call(end_done);
	(void)future_result().call_ended; // do not care about value
}

bool gsm::accept_call() {
	using namespace sim900;

	static constexpr auto end_done = [](Result res) {
		if (res != Result::OK && res != Result::ERROR) {
			schedule(Task::REBOOT);
		}
		future_result({.call_accepted = false});
		execute_scheduled();
	};

	static constexpr auto accept_done = [](Result res) {
		if (res == Result::OK && actual_call_state != CallState::RINGING) {
			// accepted a call which is not currently handled(next which started immediately after) - reject it
			sim900::end_call(end_done);
		} else {
			if (res != Result::OK && res != Result::ERROR) {
				schedule(Task::REBOOT);
			}
			future_result({.call_accepted = res == Result::OK});
			execute_scheduled();
		}
	};

	sim900::accept_call(accept_done);
	return future_result().call_accepted;
}

gsm::Dialing gsm::call(char * number) {
	using namespace sim900;

	static constexpr auto end_done = [](Result res) {
		if (res != Result::OK && res != Result::ERROR) {
			schedule(Task::REBOOT);
		}
		future_result({.dialing = Dialing::ERROR});
		execute_scheduled();
	};

	static constexpr auto call_done = [](Result res) {
		if (res == Result::OK && handled_call_state != CallState::ENDED) {
			// making a call now will break sequence of "call dialed" -> "call ended" callbacks
			// end it leaving "actual state" ENDED - "call state changed" and "call ended" callbacks will ignore
			sim900::end_call(end_done);
		} else {
			if (res == Result::OK) {
				actual_call_state = CallState::RINGING;
				// "actual state" is ENDED(otherwise call would not be established),
				// "handled state" also ENDED - may write, no racing with other precess
				handled_call_state = CallState::RINGING;
				call_direction = CallDirection::OUTGOING;
				// future result is set in "call state changed" callback
			} else {
				// didn't establish a call
				if (res != Result::ERROR) {
					schedule(Task::REBOOT);
				}
				future_result({.dialing = Dialing::ERROR});
			}
			execute_scheduled();
		}
	};

	sim900::call(number, call_done);
	return future_result().dialing;
}

static void call_dialed(gsm::Dialing dialing) {
	using namespace gsm;
	using sim900::CallState;

	if (dialing == Dialing::DONE) {
		handle(Event::CALL_STATE_CHANGED); // to invoke "on dialed" callback
	} else {
		handled_call_state = CallState::ENDED;
	}
	future_result({.dialing = dialing});
}

void sim900::on_call_update(CallState state, CallDirection direction, char * number) {
	using namespace gsm;
	if (direction == CallDirection::INCOMING) {
		// start new incoming if end of previous is already handled
		if (state == CallState::RINGING && handled_call_state == CallState::ENDED) {
			actual_call_state = CallState::RINGING;
			call_direction = direction;
			strcpy(phone_number, number);
			handle(Event::CALL_STATE_CHANGED);
			return;
		}
		if (state == CallState::SPEAKING && actual_call_state == CallState::RINGING) {
			actual_call_state = CallState::SPEAKING;
			handle(Event::CALL_STATE_CHANGED);
			return;
		}
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