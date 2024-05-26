//
// Created by independent-variable on 5/20/2024.
//

#pragma once

#define MODULE_STATUS_UPDATE_PERIOD_ms	10'000U
/** In case mobile network connection loss, how long to wait before GSM module reboot.
 * Having this delay limits rate of connection attempts. */
#define CONNECTION_RECOVERY_TIME_ms		60'000U
/** Time between starting outgoing call and getting result of dialing: answered/rejected/busy/etc.
 * Should be longer that SIM900's timeout for "NO ANSWER" */
#define DIALING_TIMEOUT_ms				120'000U