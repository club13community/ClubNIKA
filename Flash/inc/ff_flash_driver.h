//
// Created by independent-variable on 4/15/2024.
//

#pragma once
#include "diskio.h"

namespace flash {
	void init_disk_driver();

	DSTATUS disk_initialize();

	DSTATUS disk_status();

	DRESULT disk_read(BYTE *buff, DWORD sector, UINT count);

	DRESULT disk_write(const BYTE *buff, DWORD sector, UINT count);

	DRESULT disk_ioctl(BYTE cmd, void *buff);

#if FF_USE_MKFS == 1
	/** Formats flash.
	 * @param buffer 512 bytes buffer
	 * @param root_entries_x16 number of possible root dir. entries multiplied by 16 */
	FRESULT make_fs(uint8_t * buffer, uint8_t root_entries_x16);
#endif

}