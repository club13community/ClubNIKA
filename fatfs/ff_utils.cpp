//
// Created by independent-variable on 6/12/2024.
//
#include "ff_utils.h"

char * copy_filename(const char * path, char * dst) {
	// move from start to end and capture last '/' and end of string
	const char * name_start = path; // ok, if '/' - see below
	const char * src = path;
	char c;
	while ((c = *src) != '\0') {
		src++;
		if (c == '/') {
			name_start = src; // already next after '/'
		}
	}
	// copy name of file
	src = name_start;
	while ((c = *src++) != '\0') {
		*dst++ = c;
	}
	*dst = '\0';
	return dst;
}

FRESULT copy_file(FIL * src, FIL * dst, uint8_t * buf, uint16_t buf_len) {
	UINT read_len, written_len;
	bool buffer_moved;
	FRESULT last_res;
	do {
		buffer_moved = (last_res = f_read(src, buf, buf_len, &read_len)) == FR_OK
				&& (last_res = f_write(dst, buf, read_len, &written_len)) == FR_OK;
	} while (buffer_moved && read_len == buf_len);
	if (last_res == FR_OK) {
		return f_sync(dst);
	} else {
		return last_res;
	}
}

FRESULT copy_from_sd_to_flash(const char * dst_path, FIL * src, FIL * dst) {
	char src_path[FF_LFN_BUF] = "/sd/";
	copy_filename(dst_path, src_path + 4U);

	uint8_t buf[128];
	FRESULT last_res;
	void((last_res = f_open(src, src_path, FA_READ)) == FR_OK
		 && (last_res = f_open(dst, dst_path, FA_WRITE | FA_OPEN_ALWAYS)) == FR_OK
		 && (last_res = copy_file(src, dst, buf, 128U))
		 && (last_res = f_close(src)) == FR_OK
		 && (last_res = f_close(dst)) == FR_OK);
	return last_res;
}