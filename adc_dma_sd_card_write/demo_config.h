/**
 * @file    demo_config.h
 * @brief   This module simply provides some definitions for configuring the demo application.
 */

#ifndef DEMO_CONFIG_H_
#define DEMO_CONFIG_H_

/* Includes ----------------------------------------------------------------------------------------------------------*/

#include "wav_header.h"

/* Public defines ----------------------------------------------------------------------------------------------------*/

// the length of the WAVE file to write to the SD card, a positive integer, long file durations will tale a long time to write
// max value 4k seconds, about 70 minutes (we will remove this limitation in the final code, limited for the demo for simplicity).
#define DEMO_CONFIG_AUDIO_FILE_LEN_IN_SECONDS (5)

#define DEMO_CONFIG_NUM_SAMPLE_RATES_TO_TEST (6)

const Wave_Header_Sample_Rate_t demo_sample_rates_to_test[DEMO_CONFIG_NUM_SAMPLE_RATES_TO_TEST] = {
    WAVE_HEADER_SAMPLE_RATE_16kHz,
    WAVE_HEADER_SAMPLE_RATE_24kHz,
    WAVE_HEADER_SAMPLE_RATE_32kHz,
    WAVE_HEADER_SAMPLE_RATE_48kHz,
    WAVE_HEADER_SAMPLE_RATE_96kHz,
    // WAVE_HEADER_SAMPLE_RATE_192kHz, // TODO: 192kHz doesn't work yet, it takes too long to finish
    WAVE_HEADER_SAMPLE_RATE_384kHz,
};

#define DEMO_CONFIG_NUM_BIT_DEPTHS_TO_TEST (2)

const Wave_Header_Bits_Per_Sample_t demo_bit_depths_to_test[DEMO_CONFIG_NUM_BIT_DEPTHS_TO_TEST] = {
    WAVE_HEADER_16_BITS_PER_SAMPLE,
    WAVE_HEADER_24_BITS_PER_SAMPLE,
};

#endif /* DEMO_CONFIG_H_ */
