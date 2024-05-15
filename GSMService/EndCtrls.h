//
// Created by independent-variable on 5/14/2024.
//

#pragma once
#include "GSMService.h"
#include "./CtrlsSet.h"

namespace gsm {
	class EndCtrls : public Controls {
	private:
		bool end = true;
	public:
		inline bool release_ctrl() {
			return end;
		}

		void power_on(Handler handler) override {
			end = false;
			CtrlsSet::inst.power_on(handler);
		}

		void power_off(Handler handler) override {
			end = false;
			CtrlsSet::inst.power_off(handler);
		}
	};
}