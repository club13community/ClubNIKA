//
// Created by independent-variable on 4/23/2024.
//
#include "stm32f10x.h"
#include "cmd_execution.h"
#include "data_exchange.h"

extern "C" void SDIO_IRQHandler() {
	uint32_t sta = SDIO->STA;
	sd::handle_cmd_irq(sta);
	sd::handle_data_irq(sta);
}