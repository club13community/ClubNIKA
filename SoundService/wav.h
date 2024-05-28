//
// Created by independent-variable on 5/28/2024.
//

#pragma once
#include <stdint.h>

namespace player {
	class Riff {
	private:
		char id[4];
		uint32_t data_size; // how many next bytes belong to this chunk
		char type[4];
	public:
		inline bool is_riff() {
			return id[0] == 'R' && id[1] == 'I' && id[2] == 'F' && id[3] == 'F';
		}

		inline bool is_wave() {
			return type[0] == 'W' && type[1] == 'A' && type[2] == 'V' && type[3] == 'E';
		}

		inline uint32_t get_file_size() {
			return data_size + 8U;
		}
	};

	class Format {
	public:
		enum class SampleType : uint8_t {INT = 1U, FLOAT = 3U, NOT_SUPPORTED = 0xFFU};
	private:
		char id[4];
		uint32_t data_size; // how many next bytes belong to this sub-chunk
		uint8_t sample_format;
		uint16_t channels;
		uint32_t sample_rate;
		uint32_t byte_rate;
		uint16_t block_size;
		uint16_t bits_per_sample;
	public:
		inline bool is_fmt() {
			return id[0] == 'f' && id[1] == 'm' && id[2] == 't';
		}

		inline SampleType get_sample_format() {
			if (sample_format == (uint8_t)SampleType::INT) {
				return SampleType::INT;
			} else if (sample_format == (uint8_t)SampleType::FLOAT) {
				return SampleType::FLOAT;
			} else {
				return SampleType::NOT_SUPPORTED;
			}
		}

		/** channels * bytes per sample */
		inline uint16_t get_block_size() {
			return block_size;
		}

		inline uint16_t get_channels() {
			return channels;
		}

		inline uint32_t get_sample_rate() {
			return sample_rate;
		}

		inline uint16_t get_bits_per_sample() {
			return bits_per_sample;
		}
	};

	class Data {
	private:
		char id[4];
		uint32_t data_size; // how many next bytes belong to this sub-chunk
	public:
		inline bool is_data() {
			return id[0] == 'd' && id[1] == 'a' && id[2] == 't' && id[3] == 'a';
		}

		inline uint32_t get_samples_size() {
			return data_size;
		}

		inline uint32_t get_samples_number(Format & format) {
			return data_size / format.get_block_size();
		}
	};
}