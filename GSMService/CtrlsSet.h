//
// Created by independent-variable on 5/14/2024.
//

#pragma once
#include "GSMService.h"

namespace gsm {
	/** Set API methods*/
	class CtrlsSet : public Controls {
	private:
		CtrlsSet() {}
	public:
		static CtrlsSet inst;
		void power_on(Handler handler) override;
		void power_off(Handler handler) override;
	};
}