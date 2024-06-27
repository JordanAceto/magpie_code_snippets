/**
 * @file      decimation_filters.h
 * @brief     A software interface for decimation filters is represented here.
 * @details   This module is used to down-sample the raw 384kHz 24-bit audio data from the ADC/DMA modules to other
 *            sample rates. It can produce 24-bit or 16-bit results.
 */

#ifndef DECIMATION_FILTER_H_
#define DECIMATION_FILTER_H_

/* Includes ----------------------------------------------------------------------------------------------------------*/

#include <stdint.h>
#include "wav_header.h"

/* Public function declarations --------------------------------------------------------------------------------------*/

/**
 * @brief `decimation_filter_init()` initializes the decimation filter module, must be called before using the filter.
 *
 * @post the filter module is initialized, calls to downsample may now be performed.
 */
void decimation_filter_init();

/**
 * @brief `decimation_filter_downsample(s, sl, d, dsr, dbd)` downsamples source buffer `s` of length `sl` and stores the
 * result in out destination buffer `d` with sample rate `dsr` and bit depth `dbd`.
 *
 * @pre `decimation_filter_init()` has been called
 *
 * @param src_384kHz_24_bit the source buffer to downsample. It is expected that this is the 384kHz 24 bit buffer filled
 * with samples from the ADC via DMA.
 *
 * @param source_len_in_bytes the length of the source buffer, expected to be the DMA buffer length defined by the DMA
 *
 * @param dest the destination buffer for the downsampled data. Must be long enough to hold the downsampled data.
 *
 * @param dest_sample_rate the enumerated sample rate to use for the downsampled data. All sample rates are valid inputs
 * except for 384kHz, since there is no downsampling to do for this rate.
 *
 * @param dest_bit_depth the enumerated bit depth to use for the downsampled data
 *
 * @post the source buffer is downsampled and stored in the destination buffer
 *
 * @retval the length of the downsampled destination buffer in bytes
 */
uint32_t decimation_filter_downsample(
    uint8_t *src_384kHz_24_bit,
    uint32_t source_len_in_bytes,
    uint8_t *dest,
    Wave_Header_Sample_Rate_t dest_sample_rate,
    Wave_Header_Bits_Per_Sample_t dest_bit_depth);

#endif /* DECIMATION_FILTER_H_ */
