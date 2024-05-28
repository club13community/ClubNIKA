/*
 * SoundService.h
 *
 *  Created on: 3 квіт. 2020 р.
 *      Author: MaxCm
 */
#include "SoundService.h"
#include "./speaker.h"
#include "ff.h"
#include "./wav.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "periph_allocation.h"

static FIL wav;
#define MAX_SAMPLE_NUM	128U
static uint8_t samples_buffer[2][MAX_SAMPLE_NUM];
static volatile uint16_t samples_number[2];
static volatile uint8_t current_buffer;
static volatile bool end_of_wav;
static void (* volatile on_finished)() = nullptr;

static TaskHandle_t task;
static SemaphoreHandle_t mutex;

static speaker::Data getData() {
	/*if (buffer1) {
		buffer1 = false;
		return {.samples = const_cast<uint8_t *>(playBuffer1), .samplesLength = points/2};
	} else {
		buffer1 = true;
		return {.samples = const_cast<uint8_t *>(playBuffer2), .samplesLength = points/2};
	}*/
}

#define STACK_SIZE	128U
static void service_task(void * args);

void player::start() {
	speaker::initPeripherals();
	speaker::selectDac();

	static StackType_t stack[STACK_SIZE];
	static StaticTask_t task_ctrl;
	task = xTaskCreateStatic(service_task, "sound", STACK_SIZE, (void *)0, TASK_NORMAL_PRIORITY, stack, &task_ctrl);

	static StaticSemaphore_t mutex_ctrl;
	mutex = xSemaphoreCreateBinaryStatic(&mutex_ctrl);
}

static bool parse_headers(uint32_t & sample_rate) {
	using namespace player;
	UINT read_count;

	Riff riff;
	if (f_read(&wav, &riff, sizeof (Riff), &read_count) != FR_OK) {
		return false;
	}
	if (read_count != sizeof (Riff) || !riff.is_riff() || !riff.is_wave()) {
		return false;
	}

	Format format;
	if (f_read(&wav, &format, sizeof (Format), &read_count) != FR_OK) {
		return false;
	}
	if (read_count != sizeof (Format) || !format.is_fmt()) {
		return false;
	}
	if (format.get_sample_format() != Format::SampleType::INT
		|| format.get_bits_per_sample() != 8 || format.get_channels() != 1) {
		return false; // todo limit sample rate
	}

	Data data;
	if(f_read(&wav, &data, sizeof(Data), &read_count) != FR_OK) {
		return false;
	}
	if (read_count != sizeof (Data) || !data.is_data() || data.get_samples_size() == 0) {
		return false;
	}

	sample_rate = format.get_sample_rate();
	return true;
}

static bool load_wav(const char * file) {
	if (f_open(&wav, file, FA_READ) != FR_OK) {
		return false;
	}
	uint32_t sample_rate;
	UINT actual_samples;
	if (!parse_headers(sample_rate)
		|| f_read(&wav, samples_buffer, MAX_SAMPLE_NUM * 2U, &actual_samples) != FR_OK
		|| actual_samples == 0) {
		f_close(&wav);
		return false;
	}
	if (actual_samples <= MAX_SAMPLE_NUM) {
		samples_number[0] = actual_samples;
		samples_number[1] = 0;
		end_of_wav = true;
	} else {
		samples_number[0] = MAX_SAMPLE_NUM;
		samples_number[1] = actual_samples - MAX_SAMPLE_NUM;
		end_of_wav = false;
	}
	current_buffer = 1U;
	// todo set sample rate
	return true;
}

static inline void finish_previous() {
	speaker::stopPlay();
	void (* finished)() = on_finished;
	if (finished != nullptr) {
		finished();
		on_finished = nullptr;
	}
}

speaker::Data get_samples() {
	uint8_t this_buff = current_buffer;
	uint8_t next_buff = this_buff ^ 0x01U;
	samples_number[this_buff] = 0;
	if (samples_number[next_buff] != 0) {
		current_buffer = next_buff;
		// todo request to load data
		return {.samples = samples_buffer[next_buff], .samplesLength = samples_number[next_buff]};
	} else {
		// end of file or next data is still not loaded
		if (end_of_wav) {
			// todo mute? & on_finish()
		} else {
			// todo request to load both and start again
		}
		// todo how to yield from ISR??
		return speaker::DATA_END;
	}
}

static inline void play_next(void (* finished)()) {
	on_finished = finished;
	speaker::playOnDac(63U, get_samples, nullptr);
}

bool player::play_via_speaker(const char * file, void (* finished)()) {
	finish_previous();
	if (!load_wav(file)) {
		return false;
	}
	speaker::selectDac();
	speaker::unmute();
	play_next(finished);
}

bool player::play_for_gsm(const char * file, void (* finished)()) {

}

void player::stop_play() {

}

bool player::is_playing() {

}

static void service_task(void * args) {

	/*speaker::unmute();
	speaker::selectDac();
	speaker::playOnDac(SAMPLING_48K, getData, end);

	return mainTask_handle;*/
	while (true);
}

