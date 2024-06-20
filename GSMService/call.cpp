//
// Created by independent-variable on 5/24/2024.
//
#include "./call.h"
#include "./execution.h"
#include "sim900_callbacks.h"
#include "./callback_handling.h"
#include "./config.h"
#include "./sim900/config.h"
#include <string.h>

namespace gsm {
	volatile CallPhase actual_call_phase = CallPhase::NONE;
	volatile Direction call_direction = Direction::INCOMING;
	char phone_number[MAX_PHONE_LENGTH + 1];
	char pressed_key;
}

static inline void change_call_phase(gsm::CallPhase new_actual) {
	using namespace gsm;
	actual_call_phase = new_actual;
	handle(Event::CALL_STATE_CHANGED);
}

void gsm::end_call() {
	using namespace sim900;

	static constexpr auto end_done = [](Result res) {
		if (is_ongoing(actual_call_phase)) {
			change_call_phase(CallPhase::ENDED);
		}
		// result is OK event if there is no ongoing call
		if (res != Result::OK && res != Result::ERROR) {
			schedule_reboot();
		}
		executed();
	};

	// never breaks sequence of "call dialed" -> "call ended" callbacks
	sim900::end_call(end_done);
}

bool gsm::accept_call() {
	using namespace sim900;

	static constexpr auto end_done = [](Result res) {
		if (res != Result::OK && res != Result::ERROR) {
			schedule_reboot();
		}
		future_result({.call_accepted = false});
		executed();
	};

	static constexpr auto accept_done = [](Result res) {
		if (res == Result::OK && !is_dialing(actual_call_phase)) {
			// accepted a call which is not currently handled(next which started immediately after) - reject it
			sim900::end_call(end_done);
		} else {
			if (res != Result::OK && res != Result::ERROR) {
				schedule_reboot();
			}
			future_result({.call_accepted = res == Result::OK});
			executed();
		}
	};

	sim900::accept_call(accept_done);
	return future_result().call_accepted;
}

gsm::Dialing gsm::call(const char * number) {
	using namespace sim900;

	if (actual_call_phase != CallPhase::NONE) {
		// SIM900 allows outgoing calls while ongoing(SPEAKING not RINGING) incoming
		// eliminate this feature here
		executed();
		return Dialing::ERROR;
	}

	static volatile bool dialed;
	static constexpr auto ended_or_dialed = [](uint8_t index, CallState state,
			CallDirection direction, const char * number) {
		return direction == CallDirection::OUTGOING
			&& ((dialed = state == CallState::SPEAKING) || state == CallState::ENDED);
	};

	static constexpr auto call_ended = [](bool ended, CallEnd end_type) {
		if (!ended) {
			// expected to be next message from SIM900
			future_result({.dialing = Dialing::REJECTED});
			schedule_reboot();
		} else if (end_type == CallEnd::NO_ANSWER || end_type == CallEnd::NORMAL) {
			future_result({.dialing = Dialing::NO_ANSWER});
		} else if (end_type == CallEnd::BUSY){
			future_result({.dialing = Dialing::REJECTED});
		} else {
			future_result({.dialing = Dialing::ERROR});
		}
		// actual state was not changed after call initiation, so it is still NONE - not need to change it
		executed();
	};

	static constexpr auto dialing_known = [](bool has_info) {
		if (!has_info) {
			// actual state was not changed after call initiation, so it is still NONE - not need to change it
			future_result({.dialing = Dialing::ERROR});
			schedule_reboot();
			executed();
		} else if (dialed) {
			change_call_phase(CallPhase::SPEAKING);
			future_result({.dialing = Dialing::DONE});
			executed();
		} else {
			sim900::wait_call_end(RESP_TIMEOUT_ms, call_ended); // why exactly failed to dial
		}
	};

	static constexpr auto call_started = [](Result res) {
		if (res == Result::OK) {
			call_direction = Direction::OUTGOING;
			// actual phase is RINGING, but don't change now
			sim900::wait_call_state(ended_or_dialed, DIALING_TIMEOUT_ms, dialing_known);
		} else {
			// didn't establish a call, maybe incoming call started
			if (res != Result::ERROR) {
				schedule_reboot();
			}
			future_result({.dialing = Dialing::ERROR});
			executed();
		}
	};

	sim900::call(number, call_started);
	return future_result().dialing;
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
		if (state == CallState::RINGING && actual_call_phase == CallPhase::NONE) {
			call_direction = Direction::INCOMING;
			strcpy(phone_number, number);
			change_call_phase(CallPhase::STARTED);
			return;
		}
		if (state == CallState::SPEAKING && is_dialing(actual_call_phase)) {
			change_call_phase(CallPhase::SPEAKING);
			return;
		}
	}
}

void sim900::on_ring() {
	using namespace gsm;
	if (actual_call_phase == CallPhase::STARTED) {
		change_call_phase(CallPhase::RINGING);
		return;
	}
}

void sim900::on_call_end(CallEnd end) {
	using namespace gsm;
	if (is_ongoing(actual_call_phase)) {
		change_call_phase(CallPhase::ENDED);
		return;
	}
}

void gsm::terminate_calls() {
	using namespace sim900;
	if (is_ongoing(actual_call_phase)) {
		change_call_phase(CallPhase::ENDED);
	}
}