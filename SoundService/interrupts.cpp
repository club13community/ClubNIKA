//
// Created by independent-variable on 3/24/2024.
//
#include "speaker.h"

extern "C" void DMA2_Channel3_IRQHandler() {
	speaker::dmaISR();
}

extern "C" void TIM7_IRQHandler() {
	speaker::timerISR();
}