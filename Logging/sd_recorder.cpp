//
// Created by independent-variable on 5/7/2024.
//
#include "./sd_recorder.h"
#include "ff.h"
#include "./config.h"

static FIL log_file;
static bool file_opened;

static inline bool normal_log_size() {
	return f_size(&log_file) >> 10 < MAX_LOG_SIZE_kB;
}

static inline bool rotate_logs() {
	// close current
	f_close(&log_file);
	// remove prev.
	FRESULT removal = f_unlink(PREVIOUS_LOG_PATH);
	if (removal != FR_OK && removal != FR_NO_FILE) {
		return false;
	}
	// rename current->prev.
	if (f_rename(CURRENT_LOG_PATH, PREVIOUS_LOG_PATH) != FR_OK) {
		return false;
	}
	// create new current.log
	return f_open(&log_file, CURRENT_LOG_PATH, FA_CREATE_NEW | FA_WRITE) == FR_OK;
}

static inline bool open_current_log() {
	return f_open(&log_file, CURRENT_LOG_PATH, FA_OPEN_APPEND | FA_WRITE) == FR_OK;
}

void rec::init_card_recorder() {
	file_opened = false;
}

void rec::write_to_card(const char * message, uint16_t len) {
	UINT written;
	file_opened = (file_opened || open_current_log())
				  && (normal_log_size() || rotate_logs())
				  && f_write(&log_file, message, len, &written) == FR_OK
				  && f_sync(&log_file) == FR_OK;
}