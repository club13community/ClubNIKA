//
// Created by independent-variable on 5/14/2024.
//

#pragma once
#include "GSMService.h"
#include "./EndCtrls.h"
#include "FreeRTOS.h"
#include "semphr.h"

namespace gsm {
	class Callbacks {
	private:
		SemaphoreHandle_t & mutex;
		union {
			Handler handler;
		} option;
	public:
		Callbacks(SemaphoreHandle_t & mutex) : mutex(mutex) {}

		inline void set(Handler callback) {
			option.handler = callback;
		}

		inline void handler() {
			EndCtrls ctrls;
			option.handler(ctrls);
			if (ctrls.release_ctrl()) {
				xSemaphoreGive(mutex);
			}
		}
	};
}