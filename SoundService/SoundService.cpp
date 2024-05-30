/*
 * SoundService.h
 *
 *  Created on: 3 квіт. 2020 р.
 *      Author: MaxCm
 */
#include "SoundService.h"
#include "./multiplexer.h"
#include "./converter.h"
#include "ff.h"
#include "./wav.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "periph_allocation.h"

static FIL wav;
#define MAX_SAMPLE_NUM	256U
static uint8_t samples_buffer[2][MAX_SAMPLE_NUM];
static volatile uint16_t samples_number[2];
static volatile uint8_t current_buffer;
static volatile bool end_of_wav = true;
static void (* volatile on_finished)() = nullptr;

static volatile TaskHandle_t task;
static volatile SemaphoreHandle_t mutex;

#define STACK_SIZE	256U
static void service_task(void * args);

static inline void take_mutex() {
	while (xSemaphoreTake(mutex, portMAX_DELAY) == pdFALSE);
}

static inline void give_mutex() {
	xSemaphoreGive(mutex);
}

void player::start() {
	player::init_converter();
	player::init_mux_pins();

	static StackType_t stack[STACK_SIZE];
	static StaticTask_t task_ctrl;
	task = xTaskCreateStatic(service_task, "sound", STACK_SIZE, (void *)0, PLAYER_TASK_PRIORITY, stack, &task_ctrl);

	static StaticSemaphore_t mutex_ctrl;
	mutex = xSemaphoreCreateBinaryStatic(&mutex_ctrl);
	give_mutex();
}

namespace player {
	enum class Request : uint32_t {
		LOAD_SAMPLES = 1U << 0,
		/** Did not load samples in time(as prev. requested) - now load both buffers and resume playing. */
		LOAD_AND_RESUME = 1U << 1,
		HANDLE_END = 1U << 2
	};

	static inline uint32_t flag_of(Request req) {
		return (uint32_t)req;
	}

	static inline void request_from_isr(Request req, BaseType_t & task_woken) {
		xTaskNotifyFromISR(task, flag_of(req), eSetBits, &task_woken);
	}
}

static inline void stop_now() {
	using namespace player;
	stop_conversion();
	mute_speaker();
	disconnect_dac_from_sim900();
	if (!end_of_wav) {
		end_of_wav = true;
		f_close(&wav);
	}
	ulTaskNotifyValueClear(task, 0xFF'FF'FF'FF);
}

static bool load_wav(const char * file);
static void provide_samples(uint8_t * & samples, uint16_t & size, BaseType_t & task_woken);

bool player::play_via_speaker(const char * file, void (* finished)()) {
	take_mutex();
	stop_now();
	bool started = false;
	if (load_wav(file)) {
		on_finished = finished;
		connect_speaker_to_dac();
		unmute_speaker();
		start_conversion(samples_buffer[0], samples_number[0], provide_samples);
		started = true;
	}
	give_mutex();
	return started;
}

bool player::play_for_gsm(const char * file, void (* finished)()) {
	take_mutex();
	stop_now();
	bool started = false;
	if (load_wav(file)) {
		on_finished = finished;
		connect_dac_to_sim900();
		start_conversion(samples_buffer[0], samples_number[0], provide_samples);
		started = true;
	}
	give_mutex();
	return started;
}

void player::stop_playing() {
	take_mutex();
	stop_now();
	give_mutex();
}

static bool parse_headers(uint32_t & sample_rate);
volatile FRESULT wav_err;
static bool load_wav(const char * file) {
	using namespace player;
	if ( (wav_err = f_open(&wav, file, FA_READ)) != FR_OK) {
		return false;
	}
	uint32_t sample_rate;
	UINT actual_samples;
	if (!parse_headers(sample_rate)
		|| ( wav_err = f_read(&wav, samples_buffer, MAX_SAMPLE_NUM * 2U, &actual_samples) ) != FR_OK
		|| actual_samples == 0) {
		f_close(&wav);
		return false;
	}
	set_sample_rate(sample_rate);
	if (actual_samples < MAX_SAMPLE_NUM) {
		samples_number[0] = actual_samples;
		samples_number[1] = 0;
		end_of_wav = true;
		f_close(&wav);
	} else {
		samples_number[0] = MAX_SAMPLE_NUM;
		samples_number[1] = actual_samples - MAX_SAMPLE_NUM;
		end_of_wav = false;
	}
	current_buffer = 0;
	return true;
}

/** @returns false if error occurred while reading a file or file contains not supported props. */
static bool parse_headers(uint32_t & sample_rate) {
	using namespace player;
	UINT read_count;

	Riff riff;
	if ( (wav_err = f_read(&wav, &riff, sizeof (Riff), &read_count) ) != FR_OK) {
		return false;
	}
	if (read_count != sizeof (Riff) || !riff.is_riff() || !riff.is_wave()) {
		return false;
	}

	Format format;
	if ( (wav_err = f_read(&wav, &format, sizeof (Format), &read_count) ) != FR_OK) {
		return false;
	}
	if (read_count != sizeof (Format) || !format.is_fmt()) {
		return false;
	}
	if (format.get_sample_format() != Format::SampleType::INT
		|| format.get_bits_per_sample() != 8 || format.get_channels() != 1
		|| format.get_sample_rate() < min_sample_rate || format.get_sample_rate() > max_sample_rate) {
		return false;
	}

	Data data;
	if( ( wav_err = f_read(&wav, &data, sizeof(Data), &read_count) ) != FR_OK) {
		return false;
	}
	if (read_count != sizeof (Data) || !data.is_data() || data.get_samples_size() == 0) {
		return false;
	}

	sample_rate = format.get_sample_rate();
	return true;
}

static void provide_samples(uint8_t * & samples, uint16_t & size, BaseType_t & task_woken) {
	using namespace player;
	uint8_t this_buff = current_buffer;
	uint8_t next_buff = this_buff ^ 0x01U;
	samples_number[this_buff] = 0;
	if (samples_number[next_buff] != 0) {
		current_buffer = next_buff;
		if (!end_of_wav) {
			request_from_isr(Request::LOAD_SAMPLES, task_woken);
		}
		samples = samples_buffer[next_buff];
		size = samples_number[next_buff];
	} else {
		// end of file or next data is still not loaded
		if (end_of_wav) {
			stop_conversion();
			mute_speaker();
			disconnect_dac_from_sim900();
			request_from_isr(Request::HANDLE_END, task_woken);
		} else {
			request_from_isr(Request::LOAD_AND_RESUME, task_woken);
		}
		samples = nullptr;
		size = 0;
	}
}

static void service_task();
static void service_task(void * args) {
	while (true) {
		service_task();
	}
}

static volatile void load_samples(uint8_t buff_ind);
static volatile uint16_t reloads = 0;
static void service_task() {
	using namespace player;
	uint32_t bits;
	while (xTaskNotifyWait(0, 0, &bits, portMAX_DELAY) == pdFALSE);

	take_mutex();
	bits = ulTaskNotifyValueClear(task, 0xFF'FF'FF'FFU);
	if (bits & flag_of(Request::LOAD_SAMPLES)) {
		uint8_t next_buff = current_buffer ^ 1U;
		load_samples(next_buff);
	}

	if (bits & flag_of(Request::LOAD_AND_RESUME)) { reloads++;
		uint8_t this_buff = current_buffer;
		uint8_t next_buf = this_buff ^ 1U;
		if (samples_number[next_buf] == 0) { // may be already loaded(just a little late)
			load_samples(next_buf);
		}
		load_samples(this_buff);
		if (samples_number[next_buf] > 0) {
			current_buffer = next_buf;
			set_samples(samples_buffer[next_buf], samples_number[next_buf]);
		} else {
			// already reached the end
			stop_conversion();
			mute_speaker();
			disconnect_dac_from_sim900();
			bits |= flag_of(Request::HANDLE_END);
		}
	}
	give_mutex();

	if (bits & flag_of(Request::HANDLE_END)) {
		void (* finished)() = on_finished;
		if (finished != nullptr) {
			on_finished = nullptr;
			finished();
		}
	}
}

static volatile void load_samples(uint8_t buff_ind) {
	if (end_of_wav) {
		return;
	}
	UINT read_num;
	if ( ( wav_err = f_read(&wav, samples_buffer[buff_ind], MAX_SAMPLE_NUM, &read_num) ) != FR_OK || read_num == 0) {
		end_of_wav = true;
		f_close(&wav);
		return;
	}
	samples_number[buff_ind] = read_num;
	if (read_num < MAX_SAMPLE_NUM) {
		end_of_wav = true;
		f_close(&wav);
	}
}

