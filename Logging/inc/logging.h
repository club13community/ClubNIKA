//
// Created by independent-variable on 5/5/2024.
//

#pragma once
#include <stdint.h>
#include <initializer_list>
#include "../Buffer.h"

namespace rec {

	void start();

	/** Helper class to pass numeric args. to logger.
	 * Usage: log("x={0}", {s(20)}) */
	class s {
	private:
		char text[12]; // opt. '-', 10 digits, tailing '\0'
	public:
		s(int32_t val);

		operator char * () {
			return text;
		}
	};

	/** Prints message to log. '{n}' in message are replaced by argument with index n(in range [0, 9]).
	 * @param msg '\0' - terminated string
	 * @param args - up to 10 signed/unsigned int or char * arguments */
	void log(const char * msg, std::initializer_list<const char *> args);

	void log(const char * msg);
}
