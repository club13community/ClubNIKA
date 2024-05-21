//
// Created by independent-variable on 5/20/2024.
//

#pragma once

#define CALL_STATUS_UPDATE_PERIOD_ms	200U
#define MODULE_STATUS_UPDATE_PERIOD_ms	1000U
/** In case mobile network connection loss, how long to wait before GSM module reboot.
 * Having this delay limits rate of connection attempts. */
#define CONNECTION_RECOVERY_TIME_ms		60'000U