
#include "wired_zones.h"
#include "wired_zones_meas.h"
#include "./periph.h"
#include "periph_allocation.h"
#include "./config.h"

static void main_task(void *pvParameters){
	while(1){
		taskYIELD();
	}
}

void wired_zones::init_periph() {
	init_dir_pin();
	init_ch_sel_pins();
}

static TaskHandle_t service;
static StaticTask_t service_ctrl;
static StackType_t service_stack[STACK_SIZE];

static volatile uint16_t ima;

void wired_zones::start() {
	service = xTaskCreateStatic(main_task, "wire sense.", STACK_SIZE, nullptr,
								TASK_NORMAL_PRIORITY, service_stack, &service_ctrl);
}

void wired_zones::process_measurement(uint16_t value) {
	ima = ((value << 1) + 100) / 200;
	__NOP();
}

wired_zones::Measurement wired_zones::select_measurement() {
	select_channel(2);
	measure_source_current();
	return Measurement::SOURCING;
}



