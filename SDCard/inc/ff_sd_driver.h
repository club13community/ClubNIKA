//
// Created by independent-variable on 5/4/2024.
// Impl. methods for FatFs
//

#pragma once
#include "diskio.h"

namespace sd {
	void init_disk_driver();

	DSTATUS disk_initialize();

	DSTATUS disk_status();

	DRESULT disk_read(BYTE *buff, DWORD sector, UINT count);

	DRESULT disk_write(const BYTE *buff, DWORD sector, UINT count);

	DRESULT disk_ioctl(BYTE cmd, void *buff);
}
