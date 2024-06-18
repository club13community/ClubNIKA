//
// Created by independent-variable on 5/9/2024.
//
#include "alarm.h"
#include "./config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "GSMService.h"
#include "settings.h"
#include "periph_allocation.h"
#include "SoundService.h"
#include "concurrent_utils.h"
#include "wired_zones.h"
#include "UserInterface.h"
#include "SupplySystem.h"
#include "ff_utils.h"
#include "logging.h"

#if TASK_NORMAL_PRIORITY >= configTIMER_TASK_PRIORITY
#error Timer API is expected to take effect before return
#endif

namespace alarm {
	enum class State : uint8_t {DISARMED = 0, ARMED, TRIGGERED, ALERTING};
	enum class Request : uint8_t {CHANGE_STATE = 1U << 0, ANSWER_CALL = 1U << 1};
}

static volatile alarm::State state;

static TaskHandle_t service_task;
static void service_alarm(void * args);
static volatile uint8_t service_requests;

static void filter_incoming_call(char * phone);
static void call_ended();

static TimerHandle_t choice_timer; // provides timeout for choosing action during a call
static void choice_timeout_elapsed(TimerHandle_t timer);

static TimerHandle_t zones_check_timer; // used to poll security zones when alarm is armed
static void check_zones(TimerHandle_t timer);

static TimerHandle_t alert_timer; // provides timeout for disarming
static void start_alerting(TimerHandle_t timer);

static void set_callbacks_for(alarm::State target);

void alarm::start() {
	constexpr size_t stack_size = 256;
	static StackType_t stack[stack_size];
	static StaticTask_t task_ctrl;
	service_task = xTaskCreateStatic(service_alarm, "alarm", stack_size,
									 nullptr, TASK_NORMAL_PRIORITY, stack, &task_ctrl);
	service_requests = 0;

	static StaticTimer_t choice_timer_ctrl;
	choice_timer = xTimerCreateStatic("alarm.menu", pdMS_TO_TICKS(CHOICE_TIMEOUT_s * 1000U), pdFALSE, nullptr,
									  choice_timeout_elapsed, &choice_timer_ctrl);

	static StaticTimer_t zones_timer_ctrl;
	zones_check_timer = xTimerCreateStatic("alarm.zones", pdMS_TO_TICKS(ZONES_CHECK_PERIOD_ms), pdTRUE, nullptr,
										   check_zones, &zones_timer_ctrl);

	static StaticTimer_t alert_timer_ctrl;
	alert_timer = xTimerCreateStatic("alarm.alert", 1U /* will be set later*/, pdFALSE, nullptr,
									 start_alerting, &alert_timer_ctrl);

	state = State::DISARMED;
	gsm::set_on_incoming_call(filter_incoming_call);
	set_callbacks_for(State::DISARMED);
	gsm::set_on_call_ended(call_ended);
}

static inline void request(alarm::Request req) {
	atomic_set(&service_requests, (uint8_t)req);
	xTaskNotify(service_task, 0, eNoAction);
}

static inline void set_state(alarm::State target) {
	state = target;
	__CLREX();
}

void alarm::arm() {
	set_state(State::ARMED);
	user_interface::alarm_armed();
	request(Request::CHANGE_STATE);
}

void alarm::disarm() {
	set_state(State::DISARMED);
	user_interface::alarm_disarmed();
	request(Request::CHANGE_STATE);
}

bool alarm::is_armed() {
	State state_now = state;
	return state_now == State::ARMED || state_now == State::TRIGGERED || state_now == State::ALERTING;
}

static inline bool set_state_if_current(alarm::State target, alarm::State expected) {
	uint8_t current;
	do {
		current = __LDREXB((uint8_t *)&state);
		if (current != (uint8_t)expected) {
			__CLREX();
			return false;
		}
	} while (__STREXB((uint8_t)target, (uint8_t *)&state));
	return true;
}

static void check_zones(TimerHandle_t timer) {
	using namespace alarm;
	if (wired_zones::get_active()) {
		if (set_state_if_current(State::TRIGGERED, State::ARMED)) {
			request(Request::CHANGE_STATE);
		}
	}
}

static void start_alerting(TimerHandle_t timer) {
	using namespace alarm;
	if (set_state_if_current(State::ALERTING, State::TRIGGERED)) {
		request(Request::CHANGE_STATE);
	}
}

static void filter_incoming_call(char * phone) {
	using namespace alarm;
	if (is_known_phone(phone)) {
		request(Request::ANSWER_CALL);
	} else {
		gsm::get_ctrl().end_call();
	}
}

static inline void notify_call_ended() {
	xTaskNotifyGive(service_task);
}

static inline void wait_call_end() {
	while (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) == pdFALSE);
}

static volatile uint8_t phone_index; // index of phone number to be called next, reset it when arming
static volatile bool option_selected; // helps all menus to accept choice only once, reset it before each call

static inline bool accept_call() {
	option_selected = false;
	return gsm::get_ctrl().accept_call();
}

static inline gsm::Dialing make_call(char * phone) {
	option_selected = false;
	return gsm::get_ctrl().call(phone);
}

/** Note: request is cleared after this method returns. */
static inline bool was_requested(alarm::Request req) {
	return atomic_get_and_clear(&service_requests, ~(uint8_t)req) & (uint8_t)req;
}

static void service_alarm(void * args) {
	using namespace alarm;
	while (true) {
		uint32_t bits;
		xTaskNotifyWait(0, 0, &bits, portMAX_DELAY);
		bool notify_triggered = false;
		do {
			if (was_requested(Request::ANSWER_CALL)) {
				if (accept_call()) {
					wait_call_end();
				}
			}

			if (was_requested(Request::CHANGE_STATE)) {
				State state_now = state;
				if (state_now == State::TRIGGERED) {
					uint32_t delay_ms = get_alarm_delay_s() * 1000U;
					xTimerStop(zones_check_timer, portMAX_DELAY);
					xTimerChangePeriod(alert_timer, pdMS_TO_TICKS(delay_ms), portMAX_DELAY); // this starts timer
				} else if (state_now == State::ALERTING) {
					supply::turn_on_siren();
					notify_triggered = true;
					phone_index = 0;
				} else { // disarmed or armed
					supply::turn_off_siren();
					notify_triggered = false;
					if (state_now == State::ARMED) {
						xTimerReset(zones_check_timer, portMAX_DELAY);
					} else {
						xTimerStop(zones_check_timer, portMAX_DELAY);
					}
					xTimerStop(alert_timer, portMAX_DELAY);
				}
				set_callbacks_for(state_now);
			}

			if (notify_triggered) {
				char phone_number[MAX_PHONE_LENGTH + 1];
				if (!get_phone(phone_index, phone_number)) {
					// everybody are notified
					notify_triggered = false;
					continue;
				}
				bool connected = gsm::get_signal_strength() > 0;
				if (!connected) {
					taskYIELD();
					continue;
				}

				bool notified = false;
				gsm::Dialing dialing = make_call(phone_number);
				if (dialing == gsm::Dialing::DONE) {
					wait_call_end();
					notified = true;
				} else if (dialing != gsm::Dialing::ERROR) {
					// user did not pick up a phone - send SMS
					notified = gsm::get_ctrl().send_sms(ALERT_TEXT, phone_number);
				}
				connected &= gsm::get_signal_strength() > 0;

				if (notified) {
					phone_index += 1U;
				} else if (connected) {
					// maybe no money for calls or mobile provider does not accept SMS from GSM module
					rec::log("Didn't notify phone ID={0}", {rec::s(phone_index)});
					phone_index += 1U;
				} else {
					// yield and try the same again later
					taskYIELD();
				}
			}
		} while (service_requests || notify_triggered);
	}
}

static void play_notice(const char * notice_wav);
static void disarmed_menu(char option);
static void armed_menu(char option);

static void set_callbacks_for(alarm::State target) {
	using namespace alarm;
	if (target == State::ARMED || target == State::TRIGGERED) {
		gsm::set_on_call_dialed([](gsm::Direction){
			play_notice(ARMED_NOTICE_WAV);
		});
		gsm::set_on_key_pressed(armed_menu);
	} else if (target == State::ALERTING) {
		gsm::set_on_call_dialed([](gsm::Direction){
			play_notice(ALERT_NOTICE_WAV);
		});
		gsm::set_on_key_pressed(armed_menu);
	} else { // disarmed
		gsm::set_on_call_dialed([](gsm::Direction){
			play_notice(DISARMED_NOTICE_WAV);
		});
		gsm::set_on_key_pressed(disarmed_menu);
	}
}

static void start_choice_timeout() {
	xTimerReset(choice_timer, portMAX_DELAY);
}

static void play_notice(const char * notice_wav) {
	if (!player::play_for_gsm(notice_wav, start_choice_timeout)) {
		start_choice_timeout();
	}
}

static void end_call() {
	gsm::get_ctrl().end_call();
}

static void disarmed_menu(char option) {
	if (option_selected) {
		return;
	}
	if (option != '1') {
		return;
	}
	option_selected = true;
	alarm::arm();
	if (!player::play_for_gsm(CONFIRM_ARMED_WAV, end_call)) {
		end_call();
	}
}

static void armed_menu(char option) {
	if (option_selected) {
		return;
	}
	if (option != '1') {
		return;
	}
	option_selected = true;
	alarm::disarm();
	if (!player::play_for_gsm(CONFIRM_DISARMED_WAV, end_call)) {
		end_call();
	}
}

static void call_ended() {
	xTimerStop(choice_timer, portMAX_DELAY);
	player::stop_playing();
	notify_call_ended();
}

static void choice_timeout_elapsed(TimerHandle_t timer) {
	gsm::get_ctrl().end_call();
}

FRESULT alarm::copy_wav_to_flash(FIL * src, FIL * dst) {
	FRESULT last_res;
	void((last_res = copy_from_sd_to_flash(ARMED_NOTICE_WAV, src, dst)) == FR_OK
		 && (last_res = copy_from_sd_to_flash(DISARMED_NOTICE_WAV, src, dst)) == FR_OK
		 && (last_res = copy_from_sd_to_flash(ALERT_NOTICE_WAV, src, dst)) == FR_OK
		 && (last_res = copy_from_sd_to_flash(CONFIRM_ARMED_WAV, src, dst)) == FR_OK
		 && (last_res = copy_from_sd_to_flash(CONFIRM_DISARMED_WAV, src, dst)) == FR_OK);
	return last_res;
}