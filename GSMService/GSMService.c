/*
 * GSMService.c
 *
 *  Created on: 3 квіт. 2020 р.
 *      Author: MaxCm
 */
#include "GSMService.h"
#include "IRQPriorityConfig.h"

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_dma.h"

//Peripherals: UART3(ext. UART)
struct GSMServiceContext{
	TaskHandle_t mainTask_handle;
	MessageBufferHandle_t msgIn, msgOut;
} gsm_context;

/*
 * Interrupt handler
 */
volatile uint16_t buf;
volatile uint8_t tx_buf[20];

void GSMService_UART_Rx_IH(){
	buf=USART_ReceiveData(USART3);
	DMA_Cmd(DMA1_Channel2, DISABLE);
	DMA_SetCurrDataCounter(DMA1_Channel2, 4);
	//USART_DMACmd(USART3, USART_DMAReq_Tx, ENABLE);
	DMA_Cmd(DMA1_Channel2, ENABLE);
}

void GSMService_initState(){

}

void GSMService_configPeripherals(){
	/*GPIO_InitTypeDef pinInitStruct;
	NVIC_InitTypeDef nvicInitStruct;
	USART_InitTypeDef uartInitStruct;
	DMA_InitTypeDef dmaInitStruct;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

	//Tx pin
	pinInitStruct.GPIO_Pin=GPIO_Pin_10;
	pinInitStruct.GPIO_Speed=GPIO_Speed_2MHz;
	pinInitStruct.GPIO_Mode=GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &pinInitStruct);

	//Rx pin
	pinInitStruct.GPIO_Pin=GPIO_Pin_11;
	pinInitStruct.GPIO_Mode=GPIO_Mode_IN_FLOATING;
	pinInitStruct.GPIO_Speed=GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &pinInitStruct);

	uartInitStruct.USART_BaudRate=19200;
	uartInitStruct.USART_HardwareFlowControl=USART_HardwareFlowControl_None;
	uartInitStruct.USART_Mode=USART_Mode_Rx | USART_Mode_Tx;
	uartInitStruct.USART_Parity=USART_Parity_No;
	uartInitStruct.USART_StopBits=USART_StopBits_1;
	uartInitStruct.USART_WordLength=USART_WordLength_8b;
	USART_Init(USART3, &uartInitStruct);
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);


	dmaInitStruct.DMA_Mode=DMA_Mode_Normal;
	dmaInitStruct.DMA_DIR=DMA_DIR_PeripheralDST;
	dmaInitStruct.DMA_M2M=DMA_M2M_Disable;
	dmaInitStruct.DMA_Priority=DMA_Priority_High;

	dmaInitStruct.DMA_MemoryBaseAddr=(uint32_t)tx_buf;
	dmaInitStruct.DMA_MemoryDataSize=DMA_MemoryDataSize_Byte;
	dmaInitStruct.DMA_MemoryInc=DMA_MemoryInc_Enable;

	dmaInitStruct.DMA_PeripheralBaseAddr=(uint32_t)&(USART3->DR);
	dmaInitStruct.DMA_PeripheralDataSize=DMA_PeripheralDataSize_Byte;
	dmaInitStruct.DMA_PeripheralInc=DMA_PeripheralInc_Disable;

	dmaInitStruct.DMA_BufferSize=4;

	DMA_Init(DMA1_Channel2, &dmaInitStruct);
	DMA_ITConfig(DMA1_Channel2, DMA_IT_TC, ENABLE);

	USART_DMACmd(USART3, USART_DMAReq_Tx, ENABLE);

	nvicInitStruct.NVIC_IRQChannel = USART3_IRQn;
	nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
	nvicInitStruct.NVIC_IRQChannelPreemptionPriority = EXTERNAL_UART_RX_PRIORITY;
	nvicInitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvicInitStruct);

	nvicInitStruct.NVIC_IRQChannel = DMA1_Channel2_IRQn;
	nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
	nvicInitStruct.NVIC_IRQChannelPreemptionPriority = EXTERNAL_UART_TX_PRIORITY;
	nvicInitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvicInitStruct);

	USART_Cmd(USART3, ENABLE);

	tx_buf[0]=1;
	tx_buf[1]=2;
	tx_buf[2]=3;
	tx_buf[3]=4;
	//USART_SendData(USART3, (uint16_t)25);
	DMA_Cmd(DMA1_Channel2, ENABLE);*/
}

void GSMService_mainTask(void *pvParameters){
	while(1){
		taskYIELD();
	}
}

TaskHandle_t GSMService_registerInOS(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	static const char mainTask_name[configMAX_TASK_NAME_LEN]="GSMService";
	static StaticTask_t mainTask_TCB;
	static StackType_t mainTask_stackBuffer[32]; //length=mainTask_stackDepth

	gsm_context.msgIn=msgIn;
	gsm_context.msgOut=msgOut;

	gsm_context.mainTask_handle=xTaskCreateStatic(GSMService_mainTask,
			mainTask_name,
			32, //mainTask_stackDepth
			(void *)0,
			1,
			mainTask_stackBuffer,
			&mainTask_TCB);

	return gsm_context.mainTask_handle;
}

TaskHandle_t GSMService_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	TaskHandle_t mainTask_handle;
	GSMService_initState();
	GSMService_configPeripherals();
	mainTask_handle=GSMService_registerInOS(msgIn, msgOut);
	return mainTask_handle;
}
