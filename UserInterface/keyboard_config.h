//
// Created by independent-variable on 3/21/2024.
//

#pragma once

// time between selecting a row and reading it
#define ROW_SETTLING_us	10U
// time between detection of pressed button and first sample
#define DEBOUNCE_250us	3U
// time between samples
#define SAMPLING_250us	3U
// max. number of samples
#define SAMPLE_COUNT_LIMIT	5U
// if key is pressed for shorter time - it's 'click', else - 'long press'
#define FAST_RELEASE_ms		1000U

#define STACK_SIZE		128U