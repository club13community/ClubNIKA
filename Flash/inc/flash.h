//
// Created by independent-variable on 3/26/2024.
//

#pragma once
#include <stdint.h>

namespace flash {
	typedef void (* Callback)();

	enum class Buffer {B1, B2};

	/** 10 LSB bits */
	typedef uint16_t ByteOffset;
	/** 13 LSB bits*/
	typedef uint16_t PageAddress;
	/** Address of a byte in memory*/
	struct MemoryAddress {
		PageAddress page;
		ByteOffset byte;
	};

	void init();
	void read_memory(MemoryAddress address, uint16_t length, uint8_t * destination, Callback callback);
	void read_buffer(Buffer buffer, ByteOffset address, uint16_t length, uint8_t * destination, Callback callback);
	void write_buffer(Buffer buffer, ByteOffset address, uint16_t length, uint8_t * source, Callback callback);
	void memory_to_buffer(PageAddress page, Buffer buffer, Callback callback);
	void erase_and_program(Buffer buffer, PageAddress page, Callback callback);
}