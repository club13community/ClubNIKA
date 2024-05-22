//
// Created by independent-variable on 5/21/2024.
//

#pragma once
#include "GSMService.h"
#include "sim900.h"
#include "settings.h"

#include "semphr.h"
#include "task.h"
#include "timers.h"

namespace gsm {
	/** Phases of handling a call. Almost everything what changes phase of handling should do it like
	 * "set DIALING if FREE now". */ // todo change this
	enum class CallHandling : uint8_t {FREE, DIALING, SPEAKING, ENDING};

	extern void (* volatile  on_incoming_call)(char *);
	extern void (* volatile on_call_ended)();

	extern volatile sim900::CardStatus card_status;
	extern volatile sim900::Registration registration;
	extern volatile uint8_t signal_strength;

	extern CallHandling call_handling;
	extern char phone_number[MAX_PHONE_LENGTH + 1];
	extern volatile sim900::CallEnd call_end;

	extern SemaphoreHandle_t ctrl_mutex;

	void reboot_state();

	/** Sets call_handling only if it's current value is val_now. */
	bool set_call_handing_if(CallHandling new_val, CallHandling val_now);
	/** Sets call_handling only if it's current value is val1_now or val1_now. */
	bool set_call_handling_if(CallHandling new_val, CallHandling val1_now, CallHandling val2_now);
	void set_call_handling(CallHandling new_val);
	CallHandling get_call_handling();
}