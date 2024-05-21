//
// Created by independent-variable on 5/11/2024.
//

#pragma once
#include <stdint.h>

namespace sim900 {
	/** Circular exchange buffer between 1 reader and 1 writer. Distinguishes <CR><LF> as message separator and
	 * "> "(without following <CR><LF>) as a hint for next action. */
	template <uint16_t size_pow2> class RxBuffer {
		static_assert(size_pow2 > 2);
	private:
		static constexpr uint8_t fifo_in = 0, fifo_mid = 1, fifo_out = 2;
		static constexpr uint8_t fifo_has_in = 0x01U, fifo_has_mid = 0x02U, fifo_has_out = 0x04U;
		static constexpr uint8_t fifo_empty = 0x00U;
		static constexpr uint16_t size = 1U << size_pow2;
		static constexpr uint16_t index_mask = size - 1;

		// most recently received chars; fifo[0] - fifo input(just received), fifo[2] - fifo output)
		unsigned char fifo[3];
		uint8_t fifo_level = 0;
		char data[size];
		// where message starts, points on message length(2 bytes, if 0 - message is corrupted)
		volatile uint16_t write_meta = 0;
		uint16_t write_index = 2; //=write_data, points on next character to write
		volatile uint16_t read_meta = 0; // similar to write_meta
		uint16_t read_data = 2; // similar to write_data
		bool all_ok = true;

		inline uint16_t get_written_len() {
			// write_index - write_meta - 2, but with underflow like for (size_pow2 + 1)-bits wide type
			return (write_index - write_meta & index_mask) - 2;
		}

		/** @param next_write is assumed to be ahead write_meta in circular buffer */
		inline uint16_t get_free_space(uint16_t next_write) {
			// next_write - read_meta with overflows
			uint16_t used_space = next_write - read_meta & index_mask;
			return size - used_space;
		}

		inline uint16_t index_shift(uint16_t index, int16_t shift) {
			return index + shift & index_mask;
		}

	public:
		/** @returns true if current message ended by just added symbol. */
		bool add(char symbol) {
			fifo[fifo_out] = fifo[fifo_mid];
			fifo[fifo_mid] = fifo[fifo_in];
			fifo[fifo_in] = symbol;
			fifo_level = (fifo_level << 1) | fifo_has_in;

			bool delim = (fifo_level & (fifo_has_in | fifo_has_mid))
						 && *(uint16_t *) fifo == ((uint16_t) '\r' << 8 | (uint16_t) '\n'); // last received: "\r\n"
			bool input = (fifo_level == (fifo_has_in | fifo_has_mid))
						 && *(uint16_t *) fifo == ((uint16_t) '>' << 8 | (uint16_t) ' '); // currently received: "> "
			bool end = delim || input;

			if (!all_ok) {
				// message is already corrupted - no reason to put it to buffer
			} else if (input) {
				// try to put "> " in buffer
				if (get_free_space(write_index) > 2) { // > 2 to have next write_index available for write
					data[write_index] = '>';
					data[index_shift(write_index, 1)] = ' ';
					write_index = index_shift(write_index, 2);
				} else {
					all_ok = false; // buffer full
				}
			} else if (fifo_level & fifo_has_out) {
				// try to put char from fifo to buffer
				uint16_t nxt = index_shift(write_index, 1);
				if (nxt != read_meta) {
					data[write_index] = fifo[fifo_out];
					write_index = nxt;
				} else {
					all_ok = false; // buffer full
				}
			}

			if (!end) {
				return false;
			}
			// finalize current message and prepare for next
			uint16_t len, nxt_meta;
			if (all_ok) {
				len = get_written_len();
				nxt_meta = index_shift(write_index, write_index & 0x1U); // half-word aligned
				// have space(meta + char) for next write(after finalization of current)?
				// note: if len == 0 - next write will take place to current slot
				all_ok &= len == 0 || !(nxt_meta == read_meta || get_free_space(nxt_meta) < 3);
			}

			if (!all_ok) {
				nxt_meta = index_shift(write_meta, 2);
				// have space(meta + char) for next write(after current fail sign finalization)?
				if (get_free_space(nxt_meta) >= 3) {
					*(uint16_t *) (data + write_meta) = 0;
					write_meta = nxt_meta;
					write_index = index_shift(nxt_meta, 2);
				} else {
					// ignore current message
					write_index = index_shift(write_meta, 2);
				}
			} else if (len > 0) {
				*(uint16_t *) (data + write_meta) = len;
				write_meta = nxt_meta;
				write_index = index_shift(nxt_meta, 2);
			} // else all_ok & len = 0 - do not change state, <CR><LF><CR><LF> between messages was received
			fifo_level = fifo_empty;
			all_ok = true;
			return true;
		}

		/** No ongoing reads/writes are allowed. */
		inline void reset() {
			fifo_level = 0;
			write_meta = 0;
			write_index = 2;
			read_meta = 0;
			read_data = 2;
			all_ok = true;
		}

		inline void mark_message_corrupted() {
			all_ok = false;
		}

		inline bool may_read() {
			return read_meta != write_meta;
		}

		inline bool is_message_corrupted() {
			return get_message_len() == 0;
		}

		/** @returns true if message is not corrupted */
		inline bool is_message_ok() {
			return get_message_len() != 0;
		}

		inline uint16_t get_message_len() {
			return *(uint16_t *) (data + read_meta);
		}

		inline char get(uint16_t offs) {
			return data[index_shift(read_data, offs)];
		}

		bool starts_with(const char *text) {
			uint16_t len = get_message_len(), i = 0;
			do {
				if (*text == '\0') {
					return true;
				} else if (*text++ != get(i++)) {
					return false;
				}
			} while (i < len);
			return *text == '\0';
		}

		bool equals(const char *text) {
			uint16_t len = get_message_len(), i = 0;
			do {
				if (*text++ != get(i++)) {
					return false;
				}
			} while (i < len);
			return *text == '\0';
		}

		/** @param start inclusive
		 * @param end exclusive*/
		inline void copy(char *text, uint16_t start, uint16_t end) {
			uint16_t len = end - start;
			for (uint16_t n = 0, i = index_shift(read_data, start); n < len; n++, i = index_shift(i, 1)) {
				*text++ = data[i];
			}
			*text = '\0';
		}

		/** @param max_len does not include tailing '\0' */
		inline uint16_t copy(char *text, uint16_t max_len) {
			uint16_t len = get_message_len();
			if (len > max_len) {
				len = max_len;
			}
			for (uint16_t n = 0, i = read_data; n < len; n++, i = index_shift(i, 1)) {
				*text++ = data[i];
			}
			*text = '\0';
			return len;
		}

		/** Many responses have format like +COMMAND: param1, param2, ...
		 * @returns number of params */
		uint8_t get_params_number() {
			uint16_t len = get_message_len();
			uint16_t i = 0;
			// move to next after ':'
			while (i < len && get(i++) != ':');
			uint8_t commas = 0;
			bool all_blank = true; // the rest of the string does not contain anything except ' '
			while (i < len) {
				char symb = get(i);
				all_blank &= symb == ' ';
				if (symb == ',') {
					commas++;
				}
				i++;
			}
			return all_blank ? 0 : commas + 1;
		}

		/** @param max_len excluding tailing '\0'
		 * @returns number of symbols in parameter*/
		uint16_t get_param(uint8_t index, char *value, uint16_t max_len) {
			uint16_t len = get_message_len();
			uint16_t i = 0;
			// move to next after ':'
			while (i < len && get(i++) != ':');
			// move to next after corresponding comma
			uint8_t commas_left = index;
			while (commas_left > 0 && i < len) {
				if (get(i++) == ',') {
					commas_left--;
				}
			}
			if (commas_left) {
				// absent param
				*value = '\0';
				return 0;
			}
			// move to first non-blank char
			while (i < len && get(i) == ' ') {
				i++;
			}

			uint16_t start = i;
			// move to next comma
			while (i < len && get(i) != ',') {
				i++;
			}
			// move back till first non-blank char
			i--;
			while (i > start && get(i) == ' ') {
				i--;
			}
			uint16_t end = i + 1;
			uint16_t par_len = end - start;
			if (par_len > max_len) {
				end = start + max_len;
				par_len = max_len;
			}
			copy(value, start, end);
			return par_len;
		}

		/** @returns true if may read again */
		inline bool next_read() {
			uint16_t rd_meta = read_meta;
			if (rd_meta == write_meta) {
				return false;
			}
			uint16_t offs = get_message_len() + 2;
			offs = offs + (offs & 0x1U); // half word aligned
			rd_meta = index_shift(rd_meta, offs);
			read_meta = rd_meta;
			read_data = index_shift(rd_meta, 2);
			return rd_meta != write_meta;
		}
	};
}