//
// Created by independent-variable on 3/17/2024.
//

#pragma once

/** time in ms for VBAT settling after connecting to 4V */
#define VBAT_SETTIMG_TIME	300
/** time in ms to discharge VBAT decoupling caps. */
#define VBAT_DISCHARGE_TIME	300

/** Size of buffer for SIM900 commands(and data)*/
#define TX_BUFFER_LENGTH	128
/** Size of circular buffer for SIM900 responses */
#define RX_BUFFER_LENGTH_pow2	9
/** Typical timeout for response from SIM900 */
#define RESP_TIMEOUT_ms 	2000U
/** Timeout for obtaining ID */
#define SEND_SMS_TIMEOUT_ms	10'000U
