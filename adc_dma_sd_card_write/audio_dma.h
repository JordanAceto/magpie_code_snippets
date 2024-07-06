/**
 * @file      audio_dma.h
 * @brief     A software interface for the audio DMA is represented here.
 * @details   This module is responsible for initializing, starting/stopping the DMA stream, and accessing the audio
 *            samples stored in the DMA buffers.
 */

#ifndef AUDIO_DMA_H_
#define AUDIO_DMA_H_

/* Includes ----------------------------------------------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

#include "wav_header.h"

/* Public definitions ------------------------------------------------------------------------------------------------*/

// 24-bit words (not bytes), note this is divisible by 2, 4, 8, 12, 16, and 24 to support all desired sample-rates
#define AUDIO_DMA_BUFF_LEN_IN_SAMPS (8256)

// the largest size in bytes that can be stored in a single round of DMA processing
#define AUDIO_DMA_LARGEST_BUFF_LEN_IN_BYTES (AUDIO_DMA_BUFF_LEN_IN_SAMPS * 4)

// the time it takes for one round of the DMA buffer to be filled, in microseconds
#define AUDIO_DMA_CHUNK_READY_PERIOD_IN_MICROSECS ((AUDIO_DMA_BUFF_LEN_IN_SAMPS * 1000) / (WAVE_HEADER_SAMPLE_RATE_384kHz / 1000))

/* Public enumerations -----------------------------------------------------------------------------------------------*/

/**
 * @brief Audio DMA errors are represented here
 */
typedef enum
{
    AUDIO_DMA_ERROR_ALL_OK,
    AUDIO_DMA_ERROR_DMA_ERROR,
} Audio_DMA_Error_t;

/* Public function declarations --------------------------------------------------------------------------------------*/

/**
 * @brief `audio_dma_init()` initializes the audio DMA stream, this must be called before calling any other DMA functions
 *
 * @post the DMA stream is initialized and ready to use
 *
 * @retval `AUDIO_DMA_ERROR_ALL_OK` if the operation succeeded, else an error code
 */
Audio_DMA_Error_t audio_dma_init();

/**
 * @brief `audio_dma_set_sample_rate(rs)` sets the internal sample-rate to `sr`, this impacts how the data is stored
 * and retrieved
 *
 * @pre the DMA is currently stopped
 *
 * @param sample_rate the sample rate to use, impacts the data size of the samples stored by DMA. For the special case
 * of 384kHz we store 3 byte samples because we don't run the 384kHz audio through any extra decimation filtering. For
 * all other sample rates we store the data as 4 byte samples, because all other sample rates are further processed by
 * the decimation filter module, which requires 4-byte q31_t as inputs.
 *
 * @post future calls to `audio_dma_consume_buffer()` will return buffers filled with samples whose size is given by
 * this relationship:
 * - 384kHz sample rate    -> (AUDIO_DMA_BUFF_LEN_IN_SAMPS * 3)
 * - any other sample rate -> (AUDIO_DMA_BUFF_LEN_IN_SAMPS * 4)
 */
void audio_dma_set_sample_rate(Wave_Header_Sample_Rate_t sample_rate);

/**
 * @brief `audio_dma_get_sample_rate()` is the current sample used by the audio DMA
 *
 * @retval the current sample rate setting used by the audio DMA
 */
Wave_Header_Sample_Rate_t audio_dma_get_sample_rate();

/**
 * @brief `audio_dma_start()` starts the audio DMA stream
 *
 * @pre DMA initialization is complete, the ADC is initialized and continuously converting
 *
 * @post the DMA stream is started and the internal buffers are continuously filled with audio data
 *
 * @retval `AUDIO_DMA_ERROR_ALL_OK` if the operation succeeded, else an error code
 */
Audio_DMA_Error_t audio_dma_start();

/**
 * @brief `audio_dma_stop()` stops the audio DMA stream
 *
 * @pre DMA initialization is complete
 *
 * @post the DMA stream is stopped, data is no longer moved into the internal buffers
 *
 * @retval `AUDIO_DMA_ERROR_ALL_OK` if the operation succeeded, else an error code
 */
Audio_DMA_Error_t audio_dma_stop();

/**
 * @brief `audio_dma_num_buffers_available()` is the number of full buffers available for reading
 *
 * @pre  DMA initialization is complete and the DMA stream has been started
 *
 * @retval the number of buffers available
 */
uint32_t audio_dma_num_buffers_available();

/**
 * @brief `audio_dma_buffer_size_in_bytes()` is the length of the buffers returned by `audio_dma_consume_buffer()` in bytes
 *
 * @retval the length of the buffers that will be returned by calls to `audio_dma_consume_buffer()`. The total length
 * depends on the currently set sample rate.
 */
uint32_t audio_dma_buffer_size_in_bytes();

/**
 * @brief `audio_dma_consume_buffer()` yields the next available buffer and reduces the number of buffers available
 *
 * @pre  DMA initialization is complete, the DMA stream has been started, and at least one buffer is available
 *
 * @post a single buffer is consumed, reducing the number of available buffers by 1
 *
 * @retval pointer to the next available buffer whose size is given by `audio_dma_buffer_size_in_bytes()`
 */
uint8_t *audio_dma_consume_buffer();

/**
 * `audio_dma_overrun_occured()` is true if a DMA overrun occured, this means that it took too long to consume the DMA
 * buffers and the data is now invalid.
 *
 * @retval true if an overrun occured
 */
bool audio_dma_overrun_occured();

/**
 * `audio_dma_clear_overrun()` clears the buffer overrun flag if it was previously set
 *
 * @post the buffer overrun flag is cleared
 */
void audio_dma_clear_overrun();

#endif /* AUDIO_DMA_H_ */
