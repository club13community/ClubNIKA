//
// Created by independent-variable on 5/7/2024.
//

#pragma once

#define MAX_MESSAGE_LENGTH		80
#define MAX_LOG_SIZE_kB			30
#define CURRENT_LOG_PATH		"/sd/current.log"
#define PREVIOUS_LOG_PATH		"/sd/previous.log"
/** Logs are written in bulks, but if this timeout elapses before enough messages are collected
 * - write as many as currently have. */
#define WAIT_BULK_TIMEOUT_min	20U
#define BULK_SIZE				512U
