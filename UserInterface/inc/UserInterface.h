/*
 * UserInterface.h
 *
 *  Created on: 3 ���. 2020 �.
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
