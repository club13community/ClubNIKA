/*
 * UARTExtension.h
 *
 *  Created on: 1 трав. 2020 р.
 *      Author: MaxCm
 */


#include "UARTExtension.h"

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_dma.h"

#include "IRQPriorityConfig.h"
#include "DMAPriorityConfig.h"

#define TX_BUFFER_SIZE	24
#define RX_BUFFER_SIZE	24

struct UARTExtensionContext{
	TaskHandle_t mainTask_handle;
	MessageBufferHandle_t msgIn, msgOut;

	uint8_t uartTxBuff[TX_BUFFER_SIZE];
	uint8_t uartRxBuff[RX_BUFFER_SIZE];
	uint8_t uartRxCount; //current number of bytes in uartRxBuff
	uint8_t uartRxLostCount; //number of lost input messages during all time(clamp at 0xFF)
	MessageBufferHandle_t uartToTaskChannel;
	uint8_t uartState;
} uartExt_context;

#define UARTState_TxBusy ((uint8_t)0x01)

#define startTxFromTask(byteNum) do{\
							taskENTER_CRITICAL();\
							uartExt_context.uartState |= UARTState_TxBusy;\
							taskEXIT_CRITICAL();\
							DMA_SetCurrDataCounter(DMA1_Channel2, byteNum);\
							DMA_Cmd(DMA1_Channel2, ENABLE);}while(0)

/*
 * Interrupt handler
 */
void UARTExtension_Rx_IH(){
	//sequence also clears ORE-flag
	uint16_t flags=USART3->SR;
	uint8_t data=(uint8_t)USART_ReceiveData(USART3);
	uint8_t finishMessage=0;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if((flags & USART_SR_ORE)){
		//finish message, inc. lost msg. counter
		finishMessage=1;
		if(uartExt_context.uartRxLostCount<(uint8_t)0xFF){
			uartExt_context.uartRxLostCount++;
		}
	}

	if((uint8_t)0==data || (uint8_t)'\n'==data){
		//finish message
		finishMessage=1;
	} else {
		if(uartExt_context.uartRxCount>=RX_BUFFER_SIZE){
			//finish message, inc. lost msg. counter
			finishMessage=1;
			if(uartExt_context.uartRxLostCount<(uint8_t)0xFF){
				uartExt_context.uartRxLostCount++;
			}
		} else {
			uartExt_context.uartRxBuff[uartExt_context.uartRxCount]=data;
			uartExt_context.uartRxCount++;
		}
	}

	if(finishMessage && uartExt_context.uartRxCount>0){
		if(0==xMessageBufferSendFromISR(uartExt_context.uartToTaskChannel,
				uartExt_context.uartRxBuff, uartExt_context.uartRxCount, &xHigherPriorityTaskWoken)){
			//inc. lost msg. counter
			if(uartExt_context.uartRxLostCount<(uint8_t)0xFF){
				uartExt_context.uartRxLostCount++;
			}
		}
		uartExt_context.uartRxCount=0;
	}
	//taskYIELD_FROM_ISR(xHigherPriorityTaskWoken) - not needed in this application
}

void UARTExtension_TxComplete_IH(){
	uint32_t currentPriority;
	DMA_Cmd(DMA1_Channel2, DISABLE);
	currentPriority=taskENTER_CRITICAL_FROM_ISR();
	uartExt_context.uartState &= ~UARTState_TxBusy;
	taskEXIT_CRITICAL_FROM_ISR(currentPriority);
}

/*
 * Main task
 * echo of UART Rx
 * send from msgIn
 */
void UARTExtension_mainTask(void *pvParameters){
	uint8_t txNum;
	while(1){
		if(uartExt_context.uartState & UARTState_TxBusy){
			//Tx busy
			taskYIELD();
		} else {
			if(pdFALSE == xMessageBufferIsEmpty(uartExt_context.uartToTaskChannel)){
				//echo
				txNum=(uint8_t)xMessageBufferReceive(uartExt_context.uartToTaskChannel, uartExt_context.uartTxBuff, TX_BUFFER_SIZE, 0);
				startTxFromTask(txNum);
			} else if(pdFALSE == xMessageBufferIsEmpty(uartExt_context.msgIn)){
				txNum=(uint8_t)xMessageBufferReceive(uartExt_context.msgIn, uartExt_context.uartTxBuff, TX_BUFFER_SIZE, 0);
				startTxFromTask(txNum);
			}
			taskYIELD();
		}
	}
}

void UARTExtension_initState(){
	uartExt_context.uartRxCount=0;
	uartExt_context.uartRxLostCount=0;
	uartExt_context.uartState=0;
	static uint8_t uartToTaskChannel_storageArea[128];
	static StaticMessageBuffer_t uartToTaskChannel_dataStruct;
	uartExt_context.uartToTaskChannel=xMessageBufferCreateStatic(128, uartToTaskChannel_storageArea,
				&uartToTaskChannel_dataStruct);
}

void UARTExtension_configPeripherals(){

	NVIC_InitTypeDef nvicInitStruct;
	USART_InitTypeDef uartInitStruct;
	DMA_InitTypeDef dmaInitStruct;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

	//Config. pins
	GPIO_InitTypeDef pinInitStruct;
	//TX - PB10
	pinInitStruct.GPIO_Pin=GPIO_Pin_10;
	pinInitStruct.GPIO_Speed=GPIO_Speed_2MHz;
	pinInitStruct.GPIO_Mode=GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &pinInitStruct);
	//Rx - PB11
	pinInitStruct.GPIO_Pin=GPIO_Pin_11;
	pinInitStruct.GPIO_Mode=GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &pinInitStruct);
	//IO(some aux.) - PE13
	pinInitStruct.GPIO_Pin=GPIO_Pin_13;
	pinInitStruct.GPIO_Speed=GPIO_Speed_2MHz;
	pinInitStruct.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_Init(GPIOE, &pinInitStruct);

	//Config. UART
	uartInitStruct.USART_BaudRate=19200;
	uartInitStruct.USART_HardwareFlowControl=USART_HardwareFlowControl_None;
	uartInitStruct.USART_Mode=USART_Mode_Rx | USART_Mode_Tx;
	uartInitStruct.USART_Parity=USART_Parity_No;
	uartInitStruct.USART_StopBits=USART_StopBits_1;
	uartInitStruct.USART_WordLength=USART_WordLength_8b;
	USART_Init(USART3, &uartInitStruct);

	//Config. Rx with IRQ
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

	nvicInitStruct.NVIC_IRQChannel = USART3_IRQn;
	nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
	nvicInitStruct.NVIC_IRQChannelPreemptionPriority = UARTEXTENSION_RX_PRIORITY;
	nvicInitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvicInitStruct);

	//Config. TX with DMA
	dmaInitStruct.DMA_Mode=DMA_Mode_Normal;
	dmaInitStruct.DMA_DIR=DMA_DIR_PeripheralDST;
	dmaInitStruct.DMA_M2M=DMA_M2M_Disable;
	dmaInitStruct.DMA_Priority=DMA1_UARTEXTENSION_TX_PRIORITY;

	dmaInitStruct.DMA_MemoryBaseAddr=(uint32_t)uartExt_context.uartTxBuff;
	dmaInitStruct.DMA_MemoryDataSize=DMA_MemoryDataSize_Byte;
	dmaInitStruct.DMA_MemoryInc=DMA_MemoryInc_Enable;

	dmaInitStruct.DMA_PeripheralBaseAddr=(uint32_t)&(USART3->DR);
	dmaInitStruct.DMA_PeripheralDataSize=DMA_PeripheralDataSize_Byte;
	dmaInitStruct.DMA_PeripheralInc=DMA_PeripheralInc_Disable;

	dmaInitStruct.DMA_BufferSize=0; //will be set before Tx

	DMA_Init(DMA1_Channel2, &dmaInitStruct);
	USART_DMACmd(USART3, USART_DMAReq_Tx, ENABLE);

	DMA_ITConfig(DMA1_Channel2, DMA_IT_TC, ENABLE);

	/*nvicInitStruct.NVIC_IRQChannel = DMA1_Channel2_IRQn;
	nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
	nvicInitStruct.NVIC_IRQChannelPreemptionPriority = UARTEXTENSION_TX_PRIORITY;
	nvicInitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvicInitStruct);*/

	USART_Cmd(USART3, ENABLE);
}

TaskHandle_t UARTExtension_registerInOS(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	//Save message channels
	uartExt_context.msgIn=msgIn;
	uartExt_context.msgOut=msgOut;

	//Create tasks
	static const char mainTask_name[configMAX_TASK_NAME_LEN]="UARTExtension";
	static StaticTask_t mainTask_TCB;
	static StackType_t mainTask_stackBuffer[64]; //length=mainTask_stackDepth

	uartExt_context.mainTask_handle=xTaskCreateStatic(UARTExtension_mainTask,
			mainTask_name,
			64, //mainTask_stackDepth
			(void *)0,
			1,
			mainTask_stackBuffer,
			&mainTask_TCB);

	return uartExt_context.mainTask_handle;
}

TaskHandle_t UARTExtension_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	TaskHandle_t mainTask_handle;
	UARTExtension_initState();
	UARTExtension_configPeripherals();
	mainTask_handle=UARTExtension_registerInOS(msgIn, msgOut);
	return mainTask_handle;
}
