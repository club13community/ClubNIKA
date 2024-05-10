/*
 * UserInterface.h
 *
 *  Created on: 3 квіт. 2020 р.
 *      Author: MaxCm
 */


#pragma once

namespace user_interface {
	void init_periph();
	void start();
	/** Notifies user interface, that alarm is armed */
	void alarm_armed();
	/** Notifies user interface, that alarm is armed */
	void alarm_disarmed();
}
