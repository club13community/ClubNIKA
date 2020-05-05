/*
 * SPIExtention.c
 *
 *  Created on: 1 трав. 2020 р.
 *      Author: MaxCm
 */

#include "SPIExtension.h"
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_gpio.h"

#include "IRQPriorityConfig.h"
#include "DMAPriorityConfig.h"

#define setCS() GPIO_SetBits(GPIOD, GPIO_Pin_7)
#define resetCS() GPIO_ResetBits(GPIOD, GPIO_Pin_7)

#define startSPIDataXchange(byteNum) do{\
								DMA_SetCurrDataCounter(DMA2_Channel1, byteNum);\
								DMA_SetCurrDataCounter(DMA2_Channel2, byteNum);\
								DMA_Cmd(DMA2_Channel1, ENABLE);\
								DMA_Cmd(DMA2_Channel2, ENABLE);}while(0)

#define finishSPIDataXchange() do{\
							DMA_Cmd(DMA2_Channel2, DISABLE);\
							DMA_Cmd(DMA2_Channel1, DISABLE);}while(0)

#define SPI_BUFFER_SIZE	4

struct SPIExtensionContext{
	TaskHandle_t mainTask_handle;
	MessageBufferHandle_t msgIn, msgOut;

	uint8_t spiBuff[SPI_BUFFER_SIZE];

} spiExt_context;


/*
 * Interrupt handlers
 */
void SPIExtension_CommFinished_IH(){
	finishSPIDataXchange();
}

/*
 * Main task
 */

void SPIExtension_mainTask(void *pvParameters){
	while(1){
		taskYIELD();
	}
}

void SPIExtension_initState(){

}

void SPIExtension_configPeripherals(){

	GPIO_InitTypeDef pinInitStruct;
	NVIC_InitTypeDef nvicInitStruct;
	SPI_InitTypeDef spiInitStruct;
	DMA_InitTypeDef dmaInitStruct;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);

	//Config. pins
	//PB5 - MOSI
	pinInitStruct.GPIO_Pin=GPIO_Pin_5;
	pinInitStruct.GPIO_Speed=GPIO_Speed_10MHz;
	pinInitStruct.GPIO_Mode=GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &pinInitStruct);
	//PB3 - SCK
	pinInitStruct.GPIO_Pin=GPIO_Pin_3;
	GPIO_Init(GPIOB, &pinInitStruct);
	//PB4 - MISO
	pinInitStruct.GPIO_Pin=GPIO_Pin_4;
	pinInitStruct.GPIO_Mode=GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &pinInitStruct);

	//PD7 - CSN
	pinInitStruct.GPIO_Pin=GPIO_Pin_7;
	pinInitStruct.GPIO_Speed=GPIO_Speed_10MHz;
	pinInitStruct.GPIO_Mode=GPIO_Mode_Out_PP;
	setCS();
	GPIO_Init(GPIOD, &pinInitStruct);

	//Config. SPI
	spiInitStruct.SPI_Mode=SPI_Mode_Master;
	spiInitStruct.SPI_CPHA=SPI_CPHA_1Edge;
	spiInitStruct.SPI_CPOL=SPI_CPOL_Low;
	spiInitStruct.SPI_NSS=SPI_NSS_Soft;
	spiInitStruct.SPI_DataSize=SPI_DataSize_8b;
	spiInitStruct.SPI_FirstBit=SPI_FirstBit_MSB;
	spiInitStruct.SPI_BaudRatePrescaler=SPI_BaudRatePrescaler_4; //3MHz-2MHz depending on clock
	spiInitStruct.SPI_Direction=SPI_Direction_2Lines_FullDuplex;
	spiInitStruct.SPI_CRCPolynomial=0;
	SPI_Init(SPI3, &spiInitStruct);
	SPI_CalculateCRC(SPI3, DISABLE);
	//Config. TX with DMA
	dmaInitStruct.DMA_Mode=DMA_Mode_Normal;
	dmaInitStruct.DMA_DIR=DMA_DIR_PeripheralDST;
	dmaInitStruct.DMA_M2M=DMA_M2M_Disable;
	dmaInitStruct.DMA_Priority=DMA2_SPIEXTENSION_PRIORITY;

	dmaInitStruct.DMA_MemoryBaseAddr=(uint32_t)spiExt_context.spiBuff;
	dmaInitStruct.DMA_MemoryDataSize=DMA_MemoryDataSize_Byte;
	dmaInitStruct.DMA_MemoryInc=DMA_MemoryInc_Enable;

	dmaInitStruct.DMA_PeripheralBaseAddr=(uint32_t)&(SPI3->DR);
	dmaInitStruct.DMA_PeripheralDataSize=DMA_PeripheralDataSize_Byte;
	dmaInitStruct.DMA_PeripheralInc=DMA_PeripheralInc_Disable;

	dmaInitStruct.DMA_BufferSize=0; //will be set before Tx/Rx
	DMA_Init(DMA2_Channel2, &dmaInitStruct);
	SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Tx, ENABLE);

	//Config. RX with DMA
	dmaInitStruct.DMA_Mode=DMA_Mode_Normal;
	dmaInitStruct.DMA_DIR=DMA_DIR_PeripheralSRC;
	dmaInitStruct.DMA_M2M=DMA_M2M_Disable;
	dmaInitStruct.DMA_Priority=DMA2_SPIEXTENSION_PRIORITY;

	dmaInitStruct.DMA_MemoryBaseAddr=(uint32_t)spiExt_context.spiBuff;
	dmaInitStruct.DMA_MemoryDataSize=DMA_MemoryDataSize_Byte;
	dmaInitStruct.DMA_MemoryInc=DMA_MemoryInc_Enable;

	dmaInitStruct.DMA_PeripheralBaseAddr=(uint32_t)&(SPI3->DR);
	dmaInitStruct.DMA_PeripheralDataSize=DMA_PeripheralDataSize_Byte;
	dmaInitStruct.DMA_PeripheralInc=DMA_PeripheralInc_Disable;

	dmaInitStruct.DMA_BufferSize=0; //will be set before Tx/Rx
	DMA_Init(DMA2_Channel1, &dmaInitStruct);
	SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Rx, ENABLE);

	DMA_ITConfig(DMA2_Channel1, DMA_IT_TC, ENABLE); //interrupt after Rx

	nvicInitStruct.NVIC_IRQChannel = DMA2_Channel1_IRQn;
	nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
	nvicInitStruct.NVIC_IRQChannelPreemptionPriority = SPIEXTENSION_SPI_PRIORITY;
	nvicInitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvicInitStruct);

	//Enable SPI
	SPI_Cmd(SPI3, ENABLE);
	SPI_NSSInternalSoftwareConfig(SPI3, SPI_NSSInternalSoft_Set); //TODO: any sense?
}

TaskHandle_t SPIExtension_registerInOS(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	//Save message channels
	spiExt_context.msgIn=msgIn;
	spiExt_context.msgOut=msgOut;

	//Create tasks
	static const char mainTask_name[configMAX_TASK_NAME_LEN]="SPIExtension";
	static StaticTask_t mainTask_TCB;
	static StackType_t mainTask_stackBuffer[32]; //length=mainTask_stackDepth

	spiExt_context.mainTask_handle=xTaskCreateStatic(SPIExtension_mainTask,
			mainTask_name,
			32, //mainTask_stackDepth
			(void *)0,
			1,
			mainTask_stackBuffer,
			&mainTask_TCB);

	return spiExt_context.mainTask_handle;
}

TaskHandle_t SPIExtension_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	TaskHandle_t mainTask_handle;
	SPIExtension_initState();
	SPIExtension_configPeripherals();
	mainTask_handle=SPIExtension_registerInOS(msgIn, msgOut);
	return mainTask_handle;
}
