//
// Created by independent-variable on 5/14/2024.
//

#pragma once
#include "sim900.h"

namespace sim900 {
	/** @param message '\0' message, latin letters only.
	 	 * @param phone '\0' ended phone number without leading '+' and country code. */
	bool send_sms(const char * message, const char * phone, ResultHandler handler); // todo define max length
	/** Deletes all SMS(sent, unread, read, etc.) */
	bool delete_all_sms(ResultHandler handler);
}