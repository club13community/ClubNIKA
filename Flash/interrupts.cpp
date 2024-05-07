//
// Created by independent-variable on 3/26/2024.
//
#include "./internal.h"

extern "C" void DMA1_Channel2_IRQHandler() {
	flash::dma_isr();
}

extern "C" void SPI1_IRQHandler() {
	flash::spi_isr();
}