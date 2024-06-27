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

#endif /* DEMO_CONFIG_H_ */
