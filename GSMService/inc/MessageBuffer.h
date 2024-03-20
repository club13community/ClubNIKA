//
// Created by independent-variable on 3/18/2024.
//

#pragma once
#include <stdint.h>
#include <exception>

/**
 * @param length should be power of 2
 */
template <uint16_t length> class MessageBuffer {
	static_assert(length != 0);
private:
	static constexpr uint16_t get_index_bits_mask() {
		uint16_t bit = 1U, mask = 0;
		while (bit < length && bit > 0) {
			mask |= bit;
			bit <<= 1U;
		}
		if (bit != length) {
			throw std::exception();
		}
		return mask;
	}
private:
	char buffer[length];
	uint16_t write_start = 0;
	uint16_t write_index = 2; // 2 bytes will contain length
	uint16_t read_start = 0;
	static constexpr uint16_t index_bits_mask = get_index_bits_mask();
public:
	MessageBuffer() {}

	uint16_t read_length() {
		return read_start == write_start ? 0 : *(uint16_t *)(buffer + read_start);
	}

	char * read_data() {
		uint16_t data_start = (read_start + 2) & index_bits_mask;
		return buffer + data_start;
	}

	void read_done() {
		if (read_start == write_start) {
			return;
		}
		uint16_t data_length = *(uint16_t *)(buffer + read_start);
		read_start = (read_start + 2 + data_length) & index_bits_mask;
	}

	void mark_all_read() {
		read_start = write_start;
	}

	bool write_char(char ch) {
		if (write_index == read_start) {
			return false;
		}
		buffer[write_index] = ch;
		write_index = (write_index + 1) & index_bits_mask;
		return true;
	}

	uint16_t end_write() {
		// As (write_index - write_start - 2)
		uint16_t write_length = (write_index + (length - write_start) + (length - 2)) & index_bits_mask;
		if (write_length == 0) {
			return write_length;
		}
		*(uint16_t *)(buffer + write_start) = write_length;
		write_start = write_index;
		write_index = (write_start + 2) & index_bits_mask;
		return write_length;
	}

	void discard_write() {
		write_index = (write_start + 2) & index_bits_mask;
	}

};