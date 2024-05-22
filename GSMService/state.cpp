//
// Created by independent-variable on 5/21/2024.
//
#include "./state.h"
#include "sim900.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "stm32f10x.h"

namespace gsm {
	void (* volatile on_incoming_call)(char *) = nullptr;

	void (* volatile on_call_ended)() = nullptr;

	volatile sim900::CardStatus card_status = sim900::CardStatus::ERROR;
	volatile sim900::Registration registration = sim900::Registration::ONGOING;
	volatile uint8_t signal_strength = 0;

	volatile CallHandling call_handling = CallHandling::FREE;
	char phone_number[MAX_PHONE_LENGTH + 1];
	volatile sim900::CallEnd call_end = sim900::CallEnd::NORMAL;

	SemaphoreHandle_t ctrl_mutex;


	void reboot_state() {
		card_status = sim900::CardStatus::ERROR;
		registration = sim900::Registration::ONGOING;
		signal_strength = 0;
		call_handling = CallHandling::FREE;
		call_end = sim900::CallEnd::NORMAL;
	}

	bool set_call_handing_if(CallHandling new_val, CallHandling val_now) {
		uint8_t * const addr = (uint8_t *)&call_handling;
		do {
			uint8_t current = __LDREXB(addr);
			if (current != (uint8_t)val_now) {
				__CLREX();
				return false;
			}
		} while (__STREXB((uint8_t)new_val, addr));
		return true;
	}

	bool set_call_handling_if(CallHandling new_val, CallHandling val1_now, CallHandling val2_now) {
		uint8_t * const addr = (uint8_t *)&call_handling;
		do {
			uint8_t current = __LDREXB(addr);
			if (current != (uint8_t)val1_now && current != (uint8_t)val2_now) {
				__CLREX();
				return false;
			}
		} while (__STREXB((uint8_t)new_val, addr));
		return true;
	}

	void set_call_handling(CallHandling new_val) {
		call_handling = new_val;
		__CLREX();
	}

	CallHandling get_call_handling() {
		return call_handling;
	}
}