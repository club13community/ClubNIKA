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
	volatile CallPhase actual_call_state = CallPhase::ENDED;
	volatile CallPhase handled_call_state = CallPhase::ENDED;
	volatile Direction call_direction = Direction::INCOMING;
	char phone_number[MAX_PHONE_LENGTH + 1];
	char pressed_key;
}

void gsm::end_call() {
	using namespace sim900;

	static constexpr auto end_done = [](Result res) {
		if (res == Result::OK || res == Result::ERROR) {
			// result is OK event if there is no ongoing call
			actual_call_state = CallPhase::ENDED;
			handled_call_state = CallPhase::ENDED;
		} else {
			schedule_reboot();
		}
		future_result({.call_ended = true});
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
			schedule_reboot();
		}
		future_result({.call_accepted = false});
		execute_scheduled();
	};

	static constexpr auto accept_done = [](Result res) {
		if (res == Result::OK && actual_call_state != CallPhase::RINGING) {
			// accepted a call which is not currently handled(next which started immediately after) - reject it
			sim900::end_call(end_done);
		} else {
			if (res != Result::OK && res != Result::ERROR) {
				schedule_reboot();
			}
			future_result({.call_accepted = res == Result::OK});
			execute_scheduled();
		}
	};

	sim900::accept_call(accept_done);
	return future_result().call_accepted;
}

gsm::Dialing gsm::call(const char * number) {
	using namespace sim900;

	if (handled_call_state != CallPhase::ENDED || actual_call_state != CallPhase::ENDED) {
		// SIM900 allows outgoing calls while active(SPEAKING to RINGING) incoming
		// eliminate this feature here
		execute_scheduled();
		return Dialing::ERROR;
	}

	static constexpr auto call_done = [](Result res) {
		if (res == Result::OK) {
			call_direction = Direction::OUTGOING;
			actual_call_state = CallPhase::RINGING;
			handle(Event::CALL_STATE_CHANGED);
			// future result is set in "call state changed" callback
		} else {
			// didn't establish a call, maybe incoming call started
			if (res != Result::ERROR) {
				schedule_reboot();
			}
			future_result({.dialing = Dialing::ERROR});
		}
		execute_scheduled();

	};

	sim900::call(number, call_done);
	return future_result().dialing;
}

static void call_dialed(gsm::Dialing dialing) {
	using namespace gsm;

	if (dialing == Dialing::DONE) {
		handle(Event::CALL_STATE_CHANGED); // to invoke "on dialed" callback
	} else {
		handled_call_state = CallPhase::ENDED;
	}
	future_result({.dialing = dialing});
}

void sim900::on_key_pressed(char key) {
	using namespace gsm;
	pressed_key = key;
	handle(Event::KEY_PRESSED);
}

void sim900::on_call_update(uint8_t index, CallState state, CallDirection direction, char * number) {
	using namespace gsm;
	if (direction == CallDirection::INCOMING) {
		// start new incoming if end of previous is already handled
		if (state == CallState::RINGING && handled_call_state == CallPhase::ENDED) {
			call_direction = Direction::INCOMING;
			actual_call_state = CallPhase::RINGING;
			strcpy(phone_number, number);
			handle(Event::CALL_STATE_CHANGED);
			return;
		}
		if (state == CallState::SPEAKING && actual_call_state == CallPhase::RINGING) {
			actual_call_state = CallPhase::SPEAKING;
			handle(Event::CALL_STATE_CHANGED);
			return;
		}
		// transition to ENDED is done in "call ended" callback
	} else { // OUTGOING
		// transition to RINGING is done in "start call" method
		if (state == CallState::SPEAKING && actual_call_state == CallPhase::RINGING) {
			actual_call_state = CallPhase::SPEAKING;
			call_dialed(Dialing::DONE);
			return;
		}
		// transition to ENDED is done in "call ended" callback
	}
}

void sim900::on_call_end(CallEnd end) {
	using namespace gsm;
	if (call_direction == Direction::INCOMING) {
		if (one_of(actual_call_state, CallPhase::RINGING, CallPhase::SPEAKING)) {
			actual_call_state = CallPhase::ENDED;
			handle(Event::CALL_STATE_CHANGED);
			return;
		}
	} else { // OUTGOING
		if (actual_call_state == CallPhase::RINGING) {
			actual_call_state = CallPhase::ENDED;
			call_dialed(end == CallEnd::NO_ANSWER || end == CallEnd::NORMAL ? Dialing::NO_ANSWER : Dialing::REJECTED);
			return;
		}
		if (actual_call_state == CallPhase::SPEAKING) {
			actual_call_state = CallPhase::ENDED;
			handle(Event::CALL_STATE_CHANGED);
			return;
		}
	}
}

void gsm::terminate_calls() {
	using namespace sim900;
	if (actual_call_state != CallPhase::ENDED) {
		sim900::on_call_end(sim900::CallEnd::NORMAL);
	}
}