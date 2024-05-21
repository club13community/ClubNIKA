#include "GSMService.h"
#include "sim900.h"
#include "sim900_callbacks.h"
#include "rtc.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "./CtrlsSet.h"
#include "./EndCtrls.h"
#include "./state.h"
#include "./tasks.h"
#include "./event_handling.h"
#include "concurrent_utils.h"

void gsm::init_periph() {
	sim900::init_periph();
}

void gsm::start() {
	static StaticSemaphore_t semaphoreBuffer;
	ctrl_mutex = xSemaphoreCreateBinaryStatic(&semaphoreBuffer);
	// mutex stays 'taken' till GSM module turned on

	init_event_handling();
	init_task_execution();

	sim900::start();
}

uint8_t gsm::get_signal_strength() {
	using namespace sim900;
	Registration reg_now = registration;
	uint8_t sing_now = signal_strength;
	return reg_now == Registration::DONE ? sing_now : 0;
}

void gsm::set_on_incoming_call(void (* callback)(char *, Controls &)) {
	on_incoming_call = callback;
}

void gsm::set_on_call_ended(void (* callback)(Controls &)) {
	on_call_ended = callback;
}

/*gsm::Controls & gsm::get_ctrl() {
	while (xSemaphoreTake(ctrl_mutex, portMAX_DELAY) == pdFALSE);
	//return CtrlsSet::inst;
}*/

void sim900::on_ring() {
	using gsm::call_state;
	if (atomic_write_if((volatile uint8_t *)&call_state, (uint8_t)CallState::RINGING, (uint8_t)CallState::ENDED)) {
		// todo start timer
	}
}

void sim900::on_timestamp(rtc::Timestamp & timestamp) {

}




