//
// Created by independent-variable on 5/1/2024.
//
#include "sd.h"
#include "sd_ctrl.h"
#include "stm32f10x.h"
#include "sd_info_private.h"
#include "card_detection.h"
#include "sd_init.h"
#include "periph.h"

using namespace sd;

static volatile bool op_ongoing;

void sd::init_periph() {
	init_sdio();
	init_sdio_pins();
	init_detect_pin();
	init_dma();
	card_present = false;
	op_ongoing = false;
}

static void card_inserted();
static void card_initialized(Error error);
static void card_removal_detected();
static void card_removed();

static inline uint32_t dis_irq() {
	uint32_t mask = __get_PRIMASK();
	__set_PRIMASK(1U);
	return mask;
}

static inline void en_irq(uint32_t mask) {
	__set_PRIMASK(mask);
}

void sd::start_periph() {
	wait_insertion(card_inserted);
}

static void card_inserted() {
	init_card(card_initialized);
}

static void card_initialized(Error error) {
	if (error == Error::NONE) {
		// card is ready
		card_present = true;
		on_card_inserted();
		uint32_t mask = dis_irq();
		if (!op_ongoing && card_present) {
			check_presence(card_removal_detected);
		}
		en_irq(mask);

	} else {
		// init. failed - wait for card removal
		// card_present stays false
		wait_removal(card_removed);
	}
}

static void card_removal_detected() {
	bool handle_removal = false;
	uint32_t mask = dis_irq();
	card_present = false;
	if (!op_ongoing) {
		handle_removal = true;
	}
	en_irq(mask);
	if (handle_removal) {
		on_card_removed();
		wait_removal(card_removed);
	}
}

static void card_removed() {
	wait_insertion(card_inserted);
}

Error sd::start_operation() {
	// capture control
	Error error;
	uint32_t mask = dis_irq();
	if (op_ongoing) {
		error = Error::CONCURRENT_ACCESS;
	} else if (!card_present) {
		error = Error::NO_CARD;
	} else {
		error = Error::NONE;
		op_ongoing = true;
	}
	en_irq(mask);
	return error;
}

void sd::end_operation(Error error) {
	bool handle_removal = false;
	uint32_t mask = dis_irq();
	if (error == Error::NONE) {
		op_ongoing = false;
		check_presence(card_removal_detected);
	} else {
		card_present = false;
		op_ongoing = false;
		handle_removal = true;
	}
	en_irq(mask);
	if (handle_removal) {
		on_card_removed();
		wait_removal(card_removed);
	}
}