//
// Created by independent-variable on 5/6/2024.
//

#pragma once
#include <stdint.h>
#include "FreeRTOS.h"
#include "event_groups.h"

namespace rec {

	/** Allows to allocate own memory for message preparation while writing. */
	template<uint8_t size_pow_2, uint16_t max_message_length> class Buffer {
	private:
		static constexpr uint16_t size = 1U << size_pow_2;
		static constexpr uint16_t index_mask = size - 1;
		static constexpr uint16_t busy_flag = 1U << 15;
		static constexpr EventBits_t may_read = 1U << 0;

	public:
		class Slot {
		private:
			char * buff;
			uint16_t slot_data;
		public:
			Slot(char * buff, uint16_t slot) : buff(buff), slot_data(slot + 2 & index_mask) {}

			Slot() : buff(nullptr), slot_data(0) {}

			inline bool is_empty() {
				return buff == nullptr;
			}

			inline void set(uint16_t index, char symb) {
				buff[slot_data + index & index_mask] = symb;
			}

			/** @param text copied till & with tailing '\0'
			* @returns index of tailing '\0' */
			inline uint16_t set(uint16_t index, const char * text) {
				char c;
				while ((c = *text++) != '\0') {
					set(index++, c);
				}
				set(index, '\0');
				return index;
			}

			inline char get(uint16_t index) const {
				return buff[slot_data + index & index_mask];
			}

			/** @returns number of chars before '\0' */
			inline uint16_t copy_to(char * text) {
				uint16_t i = 0;
				char c;
				while ((c = get(i)) != '\0') {
					*text++ = c;
					i++;
				}
				*text = '\0';
				return i;
			}

			inline uint16_t slot_start() const {
				return slot_data + (size - 2) & index_mask; // data_index - 2 with desired overflows
			}
		};
	private:
		// message slot: 2 bytes - length(if MSB = 1 - write ongoing), N bytes - message
		char buff[size];
		uint16_t new_write_slot, done_write_slot, ready_read_slot;
		StaticEventGroup_t events_ctrl;
		EventGroupHandle_t events;
	private:
		inline uint16_t get_free_space() {
			// free space = size - (new_write_slot - ready_read_slot)
			// but overflow should work for fewer bits
			uint16_t used_space = new_write_slot + (size - ready_read_slot) & index_mask;
			return size - used_space;
		}

		inline void set_slot_len(uint16_t slot_start, uint16_t len) {
			buff[slot_start] = (uint8_t)len;
			buff[slot_start + 1 & index_mask] = (uint8_t)(len >> 8);
		}

		inline uint16_t get_slot_len(uint16_t slot_start) {
			uint16_t lsb = buff[slot_start];
			uint16_t msb = buff[slot_start + 1 & index_mask];
			return msb << 8 | lsb;
		}

		inline uint16_t next_slot(uint16_t this_slot_start, uint16_t this_len) {
			return this_slot_start + this_len & index_mask;
		}

	public:
		Buffer() : new_write_slot(0), done_write_slot(0), ready_read_slot(0) {
			events = xEventGroupCreateStatic(&events_ctrl);
		}

		Slot start_write(uint16_t str_len) {
			if (str_len > max_message_length) {
				return Slot();
			}
			uint16_t slot_len = str_len + 2;
			uint16_t slot_start;
			taskENTER_CRITICAL();
			if (slot_len >= get_free_space()) {
				// '>=' to avoid new_write_slot == ready_read_slot - as it is treated as empty buffer
				taskEXIT_CRITICAL();
				return Slot();
			}
			set_slot_len(new_write_slot, slot_len | busy_flag);
			slot_start = new_write_slot;
			new_write_slot = next_slot(new_write_slot, slot_len);
			taskEXIT_CRITICAL();
			return Slot(buff, slot_start);
		}

		/** Don't invoke with 'empty' slot*/
		void end_write(Slot & slot) {
			taskENTER_CRITICAL();
			uint16_t start = slot.slot_start();
			uint16_t len = get_slot_len(start) & ~busy_flag;
			set_slot_len(start, len);
			if (done_write_slot == start) {
				do {
					done_write_slot = next_slot(done_write_slot, len);
				} while (done_write_slot != new_write_slot && ((len = get_slot_len(done_write_slot)) & busy_flag) == 0);
				xEventGroupSetBits(events, may_read);
			}
			taskEXIT_CRITICAL();
		}

		/** Blocks till have data to read */
		Slot start_read() {
			const BaseType_t clear_on_exit = pdTRUE;
			while (xEventGroupWaitBits(events, may_read, clear_on_exit, pdTRUE, portMAX_DELAY) == 0);
			return Slot(buff, ready_read_slot);
		}

		void end_read() {
			uint16_t slot_len = get_slot_len(ready_read_slot);
			ready_read_slot = next_slot(ready_read_slot, slot_len);
			if (ready_read_slot != done_write_slot) {
				xEventGroupSetBits(events, may_read);
			}
		}
	};
}