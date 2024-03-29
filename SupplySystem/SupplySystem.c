/*
 * SupplySystem.c
 *
 *  Created on: 30 бер. 2020 р.
 *      Author: MaxCm
 */
#include "SupplySystem.h"
#include "ClockControl.h"
#include "Message.h"

#include "IRQPriorityConfig.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"

//Peripherals: TIM2

#define supplyFromBattery() GPIOE->BSRR |= GPIO_BSRR_BS7
#define supplyFromSocket() GPIOE->BSRR |= GPIO_BSRR_BR7

#define enableChargeBattery() GPIOB->BSRR |= GPIO_BSRR_BS2
#define disableChargeBattery() GPIOB->BSRR |= GPIO_BSRR_BR2

#define enable12VChannel() GPIOE->BSRR |= GPIO_BSRR_BS14
#define disable12VChannel() GPIOE->BSRR |= GPIO_BSRR_BR14

#define enable12VAlarm() GPIOE->BSRR |= GPIO_BSRR_BS15
#define disable12VAlarm() GPIOE->BSRR |= GPIO_BSRR_BR15

#define isSocketVoltagePresent() (GPIOA->IDR & GPIO_IDR_IDR9)
#define isOvercurrent12V() (GPIOA->IDR & GPIO_IDR_IDR8)

struct SupplySystemContext{
	TaskHandle_t mainTask_handle;
	MessageBufferHandle_t msgIn, msgOut;

	uint32_t flags;
	uint8_t overcurrentProtState;
	uint8_t overcurrentDetectState;
	uint32_t overcurrentTimer;

} supSys_context;


//timer freq. not defined;
#define FLAG_OVERCURRENT12V_FAILURE		((uint32_t)(0x01U<<0))
#define FLAG_12VALARM_ACTIVE			((uint32_t)(0x01U<<5))
#define FLAG_12VCHANNEL_ACTIVE			((uint32_t)(0x01U<<6))

#define OVERCURRENT_PROT_STATE_BASE		(4U)
//mask
#define OVERCURRENT_PROT_STATE			(0x0FU<<OVERCURRENT_PROT_STATE_BASE)
#define OVERCURRENT_PROT_SUBSTATE_BASE	(0U)
//mask, for each state
#define OVERCURRENT_PROT_SUBSTATE		(0x0FU<<OVERCURRENT_PROT_SUBSTATE_BASE)

#define OVERCURRENT_PROT_OFF		((uint8_t)(0x00U<<OVERCURRENT_PROT_STATE_BASE))
//disable 12V supply and alarm, wait 3sec, try to enable again
#define OVERCURRENT_PROT_TRY1		((uint8_t)(0x01U<<OVERCURRENT_PROT_STATE_BASE))
//disable 12V supply and alarm, wait 3sec, try to enable again
#define OVERCURRENT_PROT_TRY2		((uint8_t)(0x02U<<OVERCURRENT_PROT_STATE_BASE))
//disable 12V supply and alarm, wait 30sec, try to enable again
#define OVERCURRENT_PROT_ON			((uint8_t)(0x03U<<OVERCURRENT_PROT_STATE_BASE))

#define OVERCURRENT_DETECT_OVERCURRENT		((uint8_t)0x01U<<0)
//mask
#define OVERCURRENT_DETECT_STATE			((uint8_t)0x03U<<4)
#define OVERCURRENT_DETECT_OFF				((uint8_t)0x00U<<4)
#define OVERCURRENT_DETECT_DEBOUNCE			((uint8_t)0x01U<<4)
#define OVERCURRENT_DETECT_OBSERVE			((uint8_t)0x02U<<4)

//20ms
#define OVERCURRENT_DEBOUNCE_TIME	20U
//1sec
#define OVERCURRENT_OBSERVE_TIME	1*1000
//3sec
#define OVERCURRENT_TRY_TIME		3*1000
//1min
#define OVERCURRENT_PROT_TIME		60*1000

 /*
  * interrupt handlers
  * */
void SupplySystem_12VChannelsOvercurrent_ProtectionReTry_SubIH();


void SupplySystem_12VChannelsOvercurrent_Detection_SubIH(){
	switch(supSys_context.overcurrentDetectState & OVERCURRENT_DETECT_STATE){
	case OVERCURRENT_DETECT_OFF:
		//nothing to do
		break;

	case OVERCURRENT_DETECT_DEBOUNCE:
		//wait timeout and check if overcurrent
		if(0==supSys_context.overcurrentTimer) {
			if(isOvercurrent12V()){
				//overcurrent detected
				supSys_context.overcurrentDetectState |= OVERCURRENT_DETECT_OVERCURRENT;
			}
			supSys_context.overcurrentDetectState &= ~OVERCURRENT_DETECT_STATE;
			supSys_context.overcurrentDetectState |= OVERCURRENT_DETECT_OFF;
		} else {
			supSys_context.overcurrentTimer--;
		}
		break;

	case OVERCURRENT_DETECT_OBSERVE:
		//observe if overcurrent during timeout
		if(isOvercurrent12V()){
			//overcurrent detected
			supSys_context.overcurrentDetectState &= ~OVERCURRENT_DETECT_STATE;
			supSys_context.overcurrentDetectState |= OVERCURRENT_DETECT_OVERCURRENT | OVERCURRENT_DETECT_OFF;
		} else {
			if(0==supSys_context.overcurrentTimer){
				supSys_context.overcurrentDetectState &= ~OVERCURRENT_DETECT_STATE;
				supSys_context.overcurrentDetectState |= OVERCURRENT_DETECT_OFF;
			} else {
				supSys_context.overcurrentTimer--;
			}
		}
		break;

	default:;
		//unexpected value
	}
}


void SupplySystem_12VChannelsOvercurrent_ProtectionOff_SubIH(){
	//normal operation
	switch(supSys_context.overcurrentProtState & OVERCURRENT_PROT_SUBSTATE){
	case 0:
		//normal operation
		if(isOvercurrent12V()){
			//launch debounce
			supSys_context.overcurrentTimer=OVERCURRENT_DEBOUNCE_TIME;
			supSys_context.overcurrentDetectState=OVERCURRENT_DETECT_DEBOUNCE; //clear other flags
			//change state
			supSys_context.overcurrentProtState &= ~OVERCURRENT_PROT_SUBSTATE;
			supSys_context.overcurrentProtState |= (1<<OVERCURRENT_PROT_SUBSTATE_BASE);
		}
		break;

	case 1:
		//during debounce
		if((supSys_context.overcurrentDetectState & OVERCURRENT_DETECT_STATE) == OVERCURRENT_DETECT_OFF){
			if(supSys_context.overcurrentDetectState & OVERCURRENT_DETECT_OVERCURRENT){
				//detected overcurrent
				supSys_context.overcurrentProtState &= ~(OVERCURRENT_PROT_STATE | OVERCURRENT_PROT_SUBSTATE);
				supSys_context.overcurrentProtState |= OVERCURRENT_PROT_TRY1 | (0<<OVERCURRENT_PROT_SUBSTATE_BASE);
				SupplySystem_12VChannelsOvercurrent_ProtectionReTry_SubIH();
			} else {
				//launch observe
				supSys_context.overcurrentTimer=OVERCURRENT_OBSERVE_TIME;
				supSys_context.overcurrentDetectState=OVERCURRENT_DETECT_OBSERVE; //clear other flags
				//change sub-state
				supSys_context.overcurrentProtState &= ~OVERCURRENT_PROT_SUBSTATE;
				supSys_context.overcurrentProtState |= (2<<OVERCURRENT_PROT_SUBSTATE_BASE);
			}
		}
		break;

	case 2:
		//during meta stability check
		if((supSys_context.overcurrentDetectState & OVERCURRENT_DETECT_STATE) == OVERCURRENT_DETECT_OFF){
			if(supSys_context.overcurrentDetectState & OVERCURRENT_DETECT_OVERCURRENT){
				//detected overcurrent
				supSys_context.overcurrentProtState &= ~(OVERCURRENT_PROT_STATE | OVERCURRENT_PROT_SUBSTATE);
				supSys_context.overcurrentProtState |= OVERCURRENT_PROT_TRY1 | (0<<OVERCURRENT_PROT_SUBSTATE_BASE);
				SupplySystem_12VChannelsOvercurrent_ProtectionReTry_SubIH();
			} else {
				//change sub-state
				supSys_context.overcurrentProtState &= ~OVERCURRENT_PROT_SUBSTATE;
				supSys_context.overcurrentProtState |= (0<<OVERCURRENT_PROT_SUBSTATE_BASE);
			}
		}
		break;

	default:;
		//unexpected value
	}
}

void SupplySystem_12VChannelsOvercurrent_ProtectionReTry_SubIH(){
	switch(supSys_context.overcurrentProtState & OVERCURRENT_PROT_SUBSTATE){
	case 0:
		//turn off 12V channels, just entered
		disable12VAlarm();
		disable12VChannel();
		//launch debounce
		supSys_context.overcurrentTimer=OVERCURRENT_DEBOUNCE_TIME;
		supSys_context.overcurrentDetectState=OVERCURRENT_DETECT_DEBOUNCE; //clear other bits
		//change sub_state
		supSys_context.overcurrentProtState &= ~OVERCURRENT_PROT_SUBSTATE;
		supSys_context.overcurrentProtState |= (1<<OVERCURRENT_PROT_SUBSTATE_BASE);
		break;

	case 1:
		//check, that overcurrent disappeared after 12V channels disabled
		if((supSys_context.overcurrentDetectState & OVERCURRENT_DETECT_STATE) == OVERCURRENT_DETECT_OFF){
			//do not check overcurrent, will be checked in next sub-state
			//launch long observe
			if((supSys_context.overcurrentProtState & OVERCURRENT_PROT_STATE) == OVERCURRENT_PROT_ON){
				supSys_context.overcurrentTimer=OVERCURRENT_PROT_TIME;
			} else {
				supSys_context.overcurrentTimer=OVERCURRENT_TRY_TIME;
			}
			supSys_context.overcurrentDetectState &= ~OVERCURRENT_DETECT_STATE;
			supSys_context.overcurrentDetectState |= OVERCURRENT_DETECT_OBSERVE; //do not clear overcurrent
			//change sub-state
			supSys_context.overcurrentProtState &= ~OVERCURRENT_PROT_SUBSTATE;
			supSys_context.overcurrentProtState |= (2<<OVERCURRENT_PROT_SUBSTATE_BASE);
		}
		break;

	case 2:
		//wait long time with disabled 12V channels, observe overcurrent; than enable 12V channels
		if((supSys_context.overcurrentDetectState & OVERCURRENT_DETECT_STATE) == OVERCURRENT_DETECT_OFF){
			if(supSys_context.overcurrentDetectState & OVERCURRENT_DETECT_OVERCURRENT){
				//still overcurrent, report failure
				uint32_t intPrio=taskENTER_CRITICAL_FROM_ISR();
				supSys_context.flags |= FLAG_OVERCURRENT12V_FAILURE;
				supSys_context.flags &= ~(FLAG_12VALARM_ACTIVE | FLAG_12VCHANNEL_ACTIVE);
				taskEXIT_CRITICAL_FROM_ISR(intPrio);
				//try to turn off again
				supSys_context.overcurrentProtState &= ~(OVERCURRENT_PROT_STATE | OVERCURRENT_PROT_SUBSTATE);
				supSys_context.overcurrentProtState |= OVERCURRENT_PROT_ON | (0<<OVERCURRENT_PROT_SUBSTATE_BASE);
			} else {
				//try to turn on 12V channels
				if(supSys_context.flags & FLAG_12VALARM_ACTIVE){
					enable12VAlarm();
				}
				if(supSys_context.flags & FLAG_12VCHANNEL_ACTIVE){
					enable12VChannel();
				}
				//launch debounce
				supSys_context.overcurrentTimer=OVERCURRENT_DEBOUNCE_TIME;
				supSys_context.overcurrentDetectState=OVERCURRENT_DETECT_DEBOUNCE;
				//change sub-state
				supSys_context.overcurrentProtState &= ~OVERCURRENT_PROT_SUBSTATE;
				supSys_context.overcurrentProtState |= (3<<OVERCURRENT_PROT_SUBSTATE_BASE);
			}
		}
		break;

	case 3:
		//debounce time with 12V channels enabled
		if((supSys_context.overcurrentDetectState & OVERCURRENT_DETECT_STATE) == OVERCURRENT_DETECT_OFF){
			if(supSys_context.overcurrentDetectState & OVERCURRENT_DETECT_OVERCURRENT){
				//overcurrent, next try
				switch(supSys_context.overcurrentProtState & OVERCURRENT_PROT_STATE){
				case OVERCURRENT_PROT_OFF:
					supSys_context.overcurrentProtState = (supSys_context.overcurrentProtState & ~(OVERCURRENT_PROT_STATE | OVERCURRENT_PROT_SUBSTATE))
						| OVERCURRENT_PROT_TRY1 | (0<<OVERCURRENT_PROT_SUBSTATE_BASE);
					break;
				case OVERCURRENT_PROT_TRY1:
					supSys_context.overcurrentProtState = (supSys_context.overcurrentProtState & ~(OVERCURRENT_PROT_STATE | OVERCURRENT_PROT_SUBSTATE))
						| OVERCURRENT_PROT_TRY2 | (0<<OVERCURRENT_PROT_SUBSTATE_BASE);
					break;
				case OVERCURRENT_PROT_TRY2:
				case OVERCURRENT_PROT_ON:
					supSys_context.overcurrentProtState = (supSys_context.overcurrentProtState & ~(OVERCURRENT_PROT_STATE | OVERCURRENT_PROT_SUBSTATE))
						| OVERCURRENT_PROT_ON | (0<<OVERCURRENT_PROT_SUBSTATE_BASE);
					break;
				default:;
					//unexpected value
				}
				SupplySystem_12VChannelsOvercurrent_ProtectionReTry_SubIH();
			} else {
				//no overcurrent, launch observe
				supSys_context.overcurrentTimer=OVERCURRENT_OBSERVE_TIME;
				supSys_context.overcurrentDetectState=OVERCURRENT_DETECT_OBSERVE;
				//change sub-state
				supSys_context.overcurrentProtState &= ~OVERCURRENT_PROT_SUBSTATE;
				supSys_context.overcurrentProtState |= (4<<OVERCURRENT_PROT_SUBSTATE_BASE);
			}
		}
		break;

	case 4:
		//short observe with 12V channels enabled
		if((supSys_context.overcurrentDetectState & OVERCURRENT_DETECT_STATE) == OVERCURRENT_DETECT_OFF){
			if(supSys_context.overcurrentDetectState & OVERCURRENT_DETECT_OVERCURRENT){
				//overcurrent, next try
				switch(supSys_context.overcurrentProtState & OVERCURRENT_PROT_STATE){
				case OVERCURRENT_PROT_OFF:
					supSys_context.overcurrentProtState = (supSys_context.overcurrentProtState & ~(OVERCURRENT_PROT_STATE | OVERCURRENT_PROT_SUBSTATE))
						| OVERCURRENT_PROT_TRY1 | (0<<OVERCURRENT_PROT_SUBSTATE_BASE);
					break;
				case OVERCURRENT_PROT_TRY1:
					supSys_context.overcurrentProtState = (supSys_context.overcurrentProtState & ~(OVERCURRENT_PROT_STATE | OVERCURRENT_PROT_SUBSTATE))
						| OVERCURRENT_PROT_TRY2 | (0<<OVERCURRENT_PROT_SUBSTATE_BASE);
					break;
				case OVERCURRENT_PROT_TRY2:
					supSys_context.overcurrentProtState = (supSys_context.overcurrentProtState & ~(OVERCURRENT_PROT_STATE | OVERCURRENT_PROT_SUBSTATE))
						| OVERCURRENT_PROT_ON | (0<<OVERCURRENT_PROT_SUBSTATE_BASE);
					break;
				case OVERCURRENT_PROT_ON:
					//nothing to do
					break;
				default:;
					//unexpected value
				}
				SupplySystem_12VChannelsOvercurrent_ProtectionReTry_SubIH();
			} else {
				//no overcurrent, again normal state
				supSys_context.overcurrentProtState &= ~(OVERCURRENT_PROT_STATE | OVERCURRENT_PROT_SUBSTATE);
				supSys_context.overcurrentProtState |= OVERCURRENT_PROT_OFF | (0<<OVERCURRENT_PROT_SUBSTATE_BASE);
			}
		}
		break;
	}
}

//overcurrent timer, TIM2, channel 1
void SupplySystem_12VChannelsOvercurrent_Timer_IH(){
		//detection block
	SupplySystem_12VChannelsOvercurrent_Detection_SubIH();

	//change state
	switch(supSys_context.overcurrentProtState & OVERCURRENT_PROT_STATE){
	case OVERCURRENT_PROT_OFF:
		SupplySystem_12VChannelsOvercurrent_ProtectionOff_SubIH();
		break;

	case OVERCURRENT_PROT_TRY1:
	case OVERCURRENT_PROT_TRY2:
	case OVERCURRENT_PROT_ON:
		SupplySystem_12VChannelsOvercurrent_ProtectionReTry_SubIH();
		break;

	default:;
		//unexpected value
	}
}


/*
 * tasks
 * */
void SupplySystem_mainTask(void *pvParameters){
	while(1){

	}
}

/*
 * initializations, should be called only once
 * */
void SupplySystem_initState(){
	supSys_context.flags=0;
	supSys_context.overcurrentDetectState=0;
	supSys_context.overcurrentProtState=0;
	supSys_context.overcurrentTimer=0;
}

void SupplySystem_configPeripherals(){
	GPIO_InitTypeDef pinInitStruct;
	NVIC_InitTypeDef nvicInitStruct;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	pinInitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	pinInitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	//PE7 - "supply from battery"
	pinInitStruct.GPIO_Pin = GPIO_Pin_7;
	GPIO_Init(GPIOE, &pinInitStruct);
	//PE14 - "12V channel" (channel1)
	pinInitStruct.GPIO_Pin = GPIO_Pin_14;
	GPIO_Init(GPIOE, &pinInitStruct);
	//PE15 - "12V alarm" (channel2)
	pinInitStruct.GPIO_Pin = GPIO_Pin_15;
	GPIO_Init(GPIOE, &pinInitStruct);
	//PB2 - "charge battery"
	pinInitStruct.GPIO_Pin = GPIO_Pin_2;
	GPIO_Init(GPIOB, &pinInitStruct);

	pinInitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	//PA9 - "socket voltage present"
	pinInitStruct.GPIO_Pin = GPIO_Pin_9;
	GPIO_Init(GPIOA, &pinInitStruct);
	//PA2 - "battery voltage"
	//analog input, will be configured during analog measurement configuration

	//PA8 - "12V channels overcurrent"
	pinInitStruct.GPIO_Pin = GPIO_Pin_8;
	GPIO_Init(GPIOA, &pinInitStruct);
	int32_t tmrFreq=getPeripheralClockFrequencyKHz(TIM2_BASE);
	if(tmrFreq<=0){
		//report critical problem; no race condition
		supSys_context.flags |= FLAG_OVERCURRENT12V_FAILURE;
		return;
	}

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	TIM_TimeBaseInitTypeDef timInitStruct;
	timInitStruct.TIM_Prescaler = 0;
	timInitStruct.TIM_Period = tmrFreq-1; //get 1ms
	timInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	timInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	timInitStruct.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM2, &timInitStruct);

	TIM_OCInitTypeDef timOcInitStruct;
	timOcInitStruct.TIM_OCMode = TIM_OCMode_Timing;
	//timOcInitStruct.the rest = no effect
	TIM_OC1Init(TIM2, &timOcInitStruct);
	TIM_SetCompare1(TIM2, tmrFreq>>1);
	TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	nvicInitStruct.NVIC_IRQChannel = TIM2_IRQn;
	nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
	nvicInitStruct.NVIC_IRQChannelPreemptionPriority = OVERCURRENT12V_TIMER_PRIORITY;
	nvicInitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvicInitStruct);

	TIM_Cmd(TIM2, ENABLE);
}

TaskHandle_t SupplySystem_registerInOS(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	//Save message channels
	supSys_context.msgIn=msgIn;
	supSys_context.msgOut=msgOut;

	static const char mainTask_name[configMAX_TASK_NAME_LEN]="SupplySystem";
	static StaticTask_t mainTask_TCB;
	static StackType_t mainTask_stackBuffer[32]; //length=mainTask_stackDepth

	// Create tasks
	supSys_context.mainTask_handle=xTaskCreateStatic(SupplySystem_mainTask,
			mainTask_name,
			32, //mainTask_stackDepth
			(void *)0,
			1,
			mainTask_stackBuffer,
			&mainTask_TCB);

	return supSys_context.mainTask_handle;
}

void SupplySystem_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	TaskHandle_t mainTask_handle;
	SupplySystem_initState();
	SupplySystem_configPeripherals();
	//mainTask_handle=SupplySystem_registerInOS(msgIn, msgOut);
	//TODO: for debug
	enable12VChannel();
	//return mainTask_handle;
}



