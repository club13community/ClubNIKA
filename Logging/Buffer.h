//
// Created by independent-variable on 5/6/2024.
//

#pragma once
#include <stdint.h>
#include "FreeRTOS.h"
#include "event_groups.h"

namespace rec {

	/** Allows to allocate own memory for message preparation while writing.
	 * @tparam bulk_size size of bulk for bulk reads. */
	template<uint8_t size_pow_2, uint16_t bulk_size, uint16_t max_message_length> class Buffer {
	private:
		static constexpr uint16_t size = 1U << size_pow_2;
		static constexpr uint16_t index_mask = size - 1;
		static constexpr uint16_t busy_flag = 1U << 15;
		static constexpr EventBits_t may_read = 1U << 0, bulk_ready = 1U << 1;

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

			/** Similar to set(uint16_t, const char *) but stops copying if the end of slot is reached;
			 * tailing '\0' will be put in any case.
			 * @param index still should be < than slot's end. */
			inline uint16_t try_set(uint16_t index, const char * text) {
				const uint16_t len = slot_length();
				char c;
				while (index < len - 1 && (c = *text++) != '\0') {
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
				return slot_data - 2 & index_mask;
			}

			inline uint16_t slot_length() {
				uint16_t start = slot_start();
				uint16_t lsb = buff[start];
				uint16_t msb = buff[start + 1U & index_mask];
				uint16_t len = msb << 8 | lsb;
				return len & ~busy_flag;
			}
		};
	private:
		// message slot: 2 bytes - length(if MSB = 1 - write ongoing), N bytes - message
		char buff[size];
		volatile uint16_t new_write_slot, done_write_slot, ready_read_slot;
		volatile uint16_t data_size; // total length of all messages which are ready for read
		StaticEventGroup_t events_ctrl;
		EventGroupHandle_t events;
	private:
		inline uint16_t get_free_space() {
			uint16_t used_space = new_write_slot - ready_read_slot & index_mask;
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

		inline uint16_t update_done_write(uint16_t from_slot, uint16_t slot_len) {
			uint16_t slot = from_slot;
			uint16_t len = slot_len;
			uint16_t extra_data = 0;
			uint16_t till_slot = new_write_slot;
			do {
				extra_data += len;
				slot = next_slot(slot, len);
			} while (slot != till_slot && ((len = get_slot_len(slot)) & busy_flag) == 0);
			done_write_slot = slot;
			return (data_size += extra_data);
		}

	public:
		Buffer() : new_write_slot(0), done_write_slot(0), ready_read_slot(0), data_size(0) {
			events = xEventGroupCreateStatic(&events_ctrl);
		}

		Slot start_write(uint16_t str_len) {
			if (str_len > max_message_length || str_len == 0) {
				return Slot();
			}
			uint16_t slot_len = str_len + 2;
			Slot slot = Slot();
			taskENTER_CRITICAL();
			if (slot_len < get_free_space()) {
				// '<' to avoid new_write_slot == ready_read_slot - as it is treated as empty buffer
				uint16_t slot_start = new_write_slot;
				set_slot_len(slot_start, slot_len | busy_flag);
				new_write_slot = next_slot(slot_start, slot_len);
				slot = Slot(buff, slot_start);
			}
			taskEXIT_CRITICAL();
			return slot;
		}

		/** Don't invoke with 'empty' slot*/
		void end_write(Slot & slot) {
			uint16_t start = slot.slot_start();
			uint16_t len = slot.slot_length();
			set_slot_len(start, len); // setting without 'busy flag'
			if (done_write_slot == start) {
				portENTER_CRITICAL();
				uint16_t data_size_now = update_done_write(start, len);
				xEventGroupSetBits(events, data_size_now < bulk_size ? may_read : may_read | bulk_ready);
				portEXIT_CRITICAL();
			}
		}

		/** Blocks execution till buffer collects data for bulk read or timeout elapsed.
		 * @returns true if has at least 1 message */
		bool wait_bulk_ready(uint32_t timeout_ms) {
			// do not clear bits
			EventBits_t bits = xEventGroupWaitBits(events, bulk_ready, pdFALSE, pdFALSE, pdMS_TO_TICKS(timeout_ms));
			return bits & may_read;
		}

		/** Blocks till have data to read */
		Slot start_read() {
			while (xEventGroupWaitBits(events, may_read, pdFALSE, pdFALSE, portMAX_DELAY) == 0); // don't clear bits
			return Slot(buff, ready_read_slot);
		}

		/** @returns empty slot if there is nothing to read. */
		Slot try_read() {
			uint16_t done_write_now = done_write_slot;
			uint16_t ready_read_now = ready_read_slot;
			return ready_read_now != done_write_now ? Slot(buff, ready_read_now) : Slot();
		}

		void end_read() {
			uint16_t slot = ready_read_slot;
			uint16_t slot_len = get_slot_len(slot);
			uint16_t nxt_slot = next_slot(slot, slot_len);
			portENTER_CRITICAL();
			uint16_t new_size = (data_size -= slot_len);
			ready_read_slot = nxt_slot;
			EventBits_t clear_bits = (new_size == 0 ? may_read : 0) | (new_size < bulk_size ? bulk_ready : 0);
			xEventGroupClearBits(events, clear_bits);
			portEXIT_CRITICAL();
		}
	};
}