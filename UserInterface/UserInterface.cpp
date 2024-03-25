/*
 * UserInterface.c
 *
 *  Created on: 3 квіт. 2020 р.
 *      Author: MaxCm
 */

#include "UserInterface.h"
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "IRQPriorityConfig.h"
#include "ClockControl.h"
#include "lcd.h"

struct UserInterfaceContext{
	TaskHandle_t mainTask_handle;
	MessageBufferHandle_t msgIn, msgOut;
} ui_context;

//TODO: implement with writes to registers
void  activateKeyboardRow(uint8_t row){

}

//PD12-PD15 - C0-C3
#define getKeyboardColumns() ((uint8_t)(~(GPIO_ReadInputData(GPIOD)>>12) & 0x0F))

/*
 * Interrupt handlers
 */

void UserInterface_Timer_IH(){

}

void UserInterface_initState(){


}

void UserInterface_configPeripherals(){
	lcd::init();
}

void UserInterface_mainTask(void *pvParameters){
	//turnOnLED();
	uint8_t col;
	activateKeyboardRow(0);
	while(1){
		col=getKeyboardColumns();
		taskYIELD();
	}
}

TaskHandle_t UserInterface_registerInOS(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	//Save message channels
	ui_context.msgIn=msgIn;
	ui_context.msgOut=msgOut;

	static const char mainTask_name[configMAX_TASK_NAME_LEN]="UserInterface";
	static StaticTask_t mainTask_TCB;
	static StackType_t mainTask_stackBuffer[32]; //length=mainTask_stackDepth

	// Create tasks
	ui_context.mainTask_handle=xTaskCreateStatic(UserInterface_mainTask,
			mainTask_name,
			32, //mainTask_stackDepth
			(void *)0,
			1,
			mainTask_stackBuffer,
			&mainTask_TCB);

	return ui_context.mainTask_handle;
}
uint8_t bat[] = {
		0b01110,
		0b11111,
		0b10001,
		0b11111,
		0b10001,
		0b11111,
		0b10001,
		0b11111
};

uint8_t sign[] = {
		0b11111,
		0b11111,
		0b00000,
		0b01110,
		0b01110,
		0b00000,
		0b00100,
		0b00100
};

TaskHandle_t UserInterface_Launch(MessageBufferHandle_t msgIn, MessageBufferHandle_t msgOut){
	TaskHandle_t mainTask_handle;
	UserInterface_initState();
	UserInterface_configPeripherals();
	mainTask_handle=UserInterface_registerInOS(msgIn, msgOut);

	using namespace lcd;
	display_on_off(Screen::ON, Cursor::OFF, Blinking::OFF);
	set_cursor_on_line1(1);
	print("I am");

	set_cursor_on_line1(7);

	create_char(0, bat);
	print('\0');

	set_cursor_on_line2(2);

	create_char(7, sign);
	print('\7');

	set_cursor_on_line2(15 - 7);
	print("working");
	backlight_on();

	return mainTask_handle;
}
