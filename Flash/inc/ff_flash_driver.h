//
// Created by independent-variable on 4/15/2024.
//

#pragma once
#include "diskio.h"

namespace flash {
	DSTATUS disk_initialize();

	DSTATUS disk_status();

	DRESULT disk_read(BYTE *buff, DWORD sector, UINT count);

	DRESULT disk_write(const BYTE *buff, DWORD sector, UINT count);

	DRESULT disk_ioctl(BYTE cmd, void *buff);
}