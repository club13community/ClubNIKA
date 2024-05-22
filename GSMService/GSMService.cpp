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

void sim900::on_ring() {
	using namespace gsm;
	execute_or_schedule(Task::GET_INCOMING_PHONE);
}

void sim900::on_call_end(CallEnd end) {
	using namespace gsm;
	execute_or_schedule(Task::NOTIFY_CALL_ENDED); // todo CallEnd::NORMAL also when outgoing and other not in network
}

void sim900::on_timestamp(rtc::Timestamp & timestamp) {

}

void end_call() {
	using namespace gsm;
	if (set_call_handling_if(CallHandling::ENDING, CallHandling::DIALING, CallHandling::SPEAKING)) {
		// todo cancel call status polling
	}
}




