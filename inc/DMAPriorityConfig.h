/*
 * DMAPriorityConfig.h
 *
 *  Created on: 1 трав. 2020 р.
 *      Author: MaxCm
 */

#ifndef _DMAPRIORITYCONFIG_H_
#define _DMAPRIORITYCONFIG_H_
#include "stm32f10x_dma.h"

#define DMA1_WIRELESSINTERFACE_PRIORITY		DMA_Priority_Medium
#define DMA1_UARTEXTENSION_TX_PRIORITY		DMA_Priority_Medium

#define DMA2_SPIEXTENSION_PRIORITY		DMA_Priority_Low

#endif /* DMAPRIORITYCONFIG_H_ */
