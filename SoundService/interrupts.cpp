//
// Created by independent-variable on 3/24/2024.
//
#include "./converter.h"

extern "C" void DMA2_Channel3_IRQHandler() {
	player::dma_isr();
}