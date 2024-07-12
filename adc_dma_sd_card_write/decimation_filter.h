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
 * `decimation_filter_set_sample_rate(sr)` sets the sample rate for the decimation filter to `sr`
 *
 * @param sample_rate the enumerated sample rate to use for following calls to `decimation_filter_downsample()`
 *
 * @post Future calls to `decimation_filter_downsample()` will use the sample rate set here until changed by calling
 * this function again with a new sample rate.
 */
void decimation_filter_set_sample_rate(Wave_Header_Sample_Rate_t sample_rate);

/**
 * @brief `decimation_filter_downsample(s, sl, d, dbd)` downsamples source buffer `s` of length `sl` and
 * stores the result in out destination buffer `d` with bit depth `dbd` and sample rate previously set by
 * `decimation_filter_set_sample_rate(sr)`
 *
 * For the special case of 384kHz this function only convertes the final bit depth, no decimation filtering is applied.
 *
 * @pre `decimation_filter_set_sample_rate(sr)` has been called wiht the desired sample rate `sr`
 *
 * @param src_384kHz the source buffer to downsample. It is expected that this is the 384kHz buffer with  the correct
 * endianness for the final wav files filled with samples from the ADC via DMA. The samples contained in this buffer
 * can be either 3 bytes wide or 4 bytes. For all output sample rates besides 384kHz we input 4 byte wide samples. For
 * the special case of 384kHz output sample rate we input 3 byte wide samples.
 *
 * @param src_len_in_bytes the length of the source buffer in bytes
 *
 * @param dest the destination buffer for the downsampled data. Must be long enough to hold the downsampled data.
 *
 * @param dest_bit_depth the enumerated bit depth to use for the downsampled data
 *
 * @post the source buffer is downsampled and stored in the destination buffer
 *
 * @retval the length of the downsampled destination buffer in bytes
 */
uint32_t decimation_filter_downsample(
    uint8_t *src_384kHz,
    uint32_t src_len_in_bytes,
    uint8_t *dest,
    Wave_Header_Bits_Per_Sample_t dest_bit_depth);

#endif /* DECIMATION_FILTER_H_ */
