//
// Created by independent-variable on 6/12/2024.
//

#pragma once
#include "ff.h"

/** Copy just file name from path to dst(with tailing '\0').
 * @returns pointer from dst to tailing '\0'. */
char * copy_filename(const char * path, char * dst);
/** Files should be already opened, after method returns files are open and dst is flushed.
 * @param buf buffer to be used while copying. */
FRESULT copy_file(FIL * src, FIL * dst, uint8_t * buf, uint16_t buf_len);
/** Copies files from SD card to flash memory.
 * Use only for initialization of flash. Uses > 128 bytes of stack.
 * @param dst_path path in flash mem.
 * @param src just buffer for file-object.
 * @param dst just buffer for file-object. */
FRESULT copy_from_sd_to_flash(const char * dst_path, FIL * src, FIL * dst);