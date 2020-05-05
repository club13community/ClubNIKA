/*
 * WirelessInterface.c
 *
 *  Created on: 1 трав. 2020 р.
 *      Author: MaxCm
 */
#include "WirelessInterface.h"
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_gpio.h"

#include "IRQPriorityConfig.h"
#include "DMAPriorityConfig.h"

#define enableCS() GPIO_ResetBits(GPIOB, GPIO_Pin_12)
#define disableCS() GPIO_SetBits(GPIOB, GPIO_Pin_12)

#define enableCE() GPIO_SetBits(GPIOD, GPIO_Pin_8)
#define disableCE() GPIO_ResetBits(GPIOD, GPIO_Pin_8)

#define startSPIComm(byteNum) do{\
								enableCS();\
								DMA_SetCurrDataCounter(DMA1_Channel4, byteNum);\
								DMA_SetCurrDataCounter(DMA1_Channel5, byteNum);\
								DMA_Cmd(DMA1_Channel4, ENABLE);\
								DMA_Cmd(DMA1_Channel5, ENABLE);}while(0)

#define finishSPIComm() do{\
							DMA_Cmd(DMA1_Channel5, DISABLE);\
							DMA_Cmd(DMA1_Channel4, DISABLE);\
							disableCS();}while(0)

#define SPI_BUFFER_SIZE	4

struct WirelessInterfaceContext{
	uint8_t spiBuff[SPI_BUFFER_SIZE];
	MessageBufferHandle_t msgIn, msgOut;
} wless_context;

/*
 * Interrupt handlers
 */
void WirelessInterface_CommFinished_IH(){
	finishSPIComm();
	__NOP();
}

void WirelessInterface_mainTask(void *pvParameters){
	while(1){

	}
}

void WirelessInterface_initState(){

}

void WirelessInterface_configPeripherals(){

	GPIO_InitTypeDef pinInitStruct;
	NVIC_InitTypeDef nvicInitStruct;
	SPI_InitTypeDef spiInitStruct;
	DMA_InitTypeDef dmaInitStruct;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	//Config. pins
	//PB15 - MOSI
	pinInitStruct.GPIO_Pin=GPIO_Pin_15;
	pinInitStruct.GPIO_Speed=GPIO_Speed_10MHz;
	pinInitStruct.GPIO_Mode=GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &pinInitStruct);
	//PB13 - SCK
	pinInitStruct.GPIO_Pin=GPIO_Pin_13;
	GPIO_Init(GPIOB, &pinInitStruct);
	//PB14 - MISO
	pinInitStruct.GPIO_Pin=GPIO_Pin_14;
	pinInitStruct.GPIO_Mode=GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &pinInitStruct);

	//PB12 - CSN
	pinInitStruct.GPIO_Pin=GPIO_Pin_12;
	pinInitStruct.GPIO_Speed=GPIO_Speed_10MHz;
	pinInitStruct.GPIO_Mode=GPIO_Mode_Out_PP;
	disableCS();
	GPIO_Init(GPIOB, &pinInitStruct);
	//PD8 - CE
	pinInitStruct.GPIO_Pin=GPIO_Pin_8;
	disableCE();
	GPIO_Init(GPIOD, &pinInitStruct);
	//PD9 - IRQ
	pinInitStruct.GPIO_Pin=GPIO_Pin_9;
	pinInitStruct.GPIO_Mode=GPIO_Mode_IN_FLOATING;
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
	SPI_Init(SPI2, &spiInitStruct);
	SPI_CalculateCRC(SPI2, DISABLE);
	//Config. TX with DMA
	dmaInitStruct.DMA_Mode=DMA_Mode_Normal;
	dmaInitStruct.DMA_DIR=DMA_DIR_PeripheralDST;
	dmaInitStruct.DMA_M2M=DMA_M2M_Disable;
	dmaInitStruct.DMA_Priority=DMA1_WIRELESSINTERFACE_PRIORITY;

	dmaInitStruct.DMA_MemoryBaseAddr=(uint32_t)wless_context.spiBuff;
	dmaInitStruct.DMA_MemoryDataSize=DMA_MemoryDataSize_Byte;
	dmaInitStruct.DMA_MemoryInc=DMA_MemoryInc_Enable;

	dmaInitStruct.DMA_PeripheralBaseAddr=(uint32_t)&(SPI2->DR);
	dmaInitStruct.DMA_PeripheralDataSize=DMA_PeripheralDataSize_Byte;
	dmaInitStruct.DMA_PeripheralInc=DMA_PeripheralInc_Disable;

	dmaInitStruct.DMA_BufferSize=0; //will be set before Tx/Rx
	DMA_Init(DMA1_Channel5, &dmaInitStruct);
	SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);

	//Config. RX with DMA
	dmaInitStruct.DMA_Mode=DMA_Mode_Normal;
	dmaInitStruct.DMA_DIR=DMA_DIR_PeripheralSRC;
	dmaInitStruct.DMA_M2M=DMA_M2M_Disable;
	dmaInitStruct.DMA_Priority=DMA1_WIRELESSINTERFACE_PRIORITY;

	dmaInitStruct.DMA_MemoryBaseAddr=(uint32_t)wless_context.spiBuff;
	dmaInitStruct.DMA_MemoryDataSize=DMA_MemoryDataSize_Byte;
	dmaInitStruct.DMA_MemoryInc=DMA_MemoryInc_Enable;

	dmaInitStruct.DMA_PeripheralBaseAddr=(uint32_t)&(SPI2->DR);
	dmaInitStruct.DMA_PeripheralDataSize=DMA_PeripheralDataSize_Byte;
	dmaInitStruct.DMA_PeripheralInc=DMA_PeripheralInc_Disable;

	dmaInitStruct.DMA_BufferSize=0; //will be set before Tx/Rx
	DMA_Init(DMA1_Channel4, &dmaInitStruct);
	SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Rx, ENABLE);

	DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE); //interrupt after Rx

	nvicInitStruct.NVIC_IRQChannel = DMA1_Channel4_IRQn;
	nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
	nvicInitStruct.NVIC_IRQChannelPreemptionPriority = WIRELESSINTERFACE_SPI_PRIORITY;
	nvicInitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvicInitStruct);

	//TODO: config interrupt for pin IRQ

	//Enable SPI
	SPI_Cmd(SPI2, ENABLE);
	SPI_NSSInternalSoftwareConfig(SPI2, SPI_NSSInternalSoft_Set); //TODO: any sense?

}

TaskHandle_t WirelessInterface_registerInOS(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	TaskHandle_t mainTask_handle;
	//Save message channels
	wless_context.msgIn=msgIn;
	wless_context.msgOut=msgOut;

	//Create tasks
	static const char mainTask_name[configMAX_TASK_NAME_LEN]="WirelessInterface";
	static StaticTask_t mainTask_TCB;
	static StackType_t mainTask_stackBuffer[32]; //length=mainTask_stackDepth

	mainTask_handle=xTaskCreateStatic(WirelessInterface_mainTask,
			mainTask_name,
			32, //mainTask_stackDepth
			(void *)0,
			1,
			mainTask_stackBuffer,
			&mainTask_TCB);
	return mainTask_handle;
}

TaskHandle_t WirelessInterface_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	TaskHandle_t mainTask_handle;
	WirelessInterface_initState();
	WirelessInterface_configPeripherals();
	mainTask_handle=WirelessInterface_registerInOS(msgIn, msgOut);
	return mainTask_handle;
}
