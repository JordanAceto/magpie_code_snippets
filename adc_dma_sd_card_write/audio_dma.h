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

/* Public definitions ------------------------------------------------------------------------------------------------*/

// 24 bit data from the ADC
#define AUDIO_DMA_SAMPLE_LEN_IN_BYTES (3)
#define AUDIO_DMA_SAMPLE_LEN_IN_BITS (AUDIO_DMA_SAMPLE_LEN_IN_BYTES * 8)

// 24-bit words (not bytes), note this is divisible by 2, 4, 8, 12, 16, and 24 to support all desired sample-rates
#define AUDIO_DMA_BUFF_LEN_IN_SAMPS (8256)
#define AUDIO_DMA_BUFF_LEN_IN_BYTES (AUDIO_DMA_SAMPLE_LEN_IN_BYTES * AUDIO_DMA_BUFF_LEN_IN_SAMPS)

// 8192 samples at 384kHz take 21.5 milliseconds to fill the buffer
#define AUDIO_DMA_CHUNK_READY_PERIOD_IN_MICROSECS (21500)

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
 * @brief `audio_dma_consume_buffer()` yields the next available buffer and reduces the number of buffers available
 *
 * @pre  DMA initialization is complete, the DMA stream has been started, and at least one buffer is available
 *
 * @post a single buffer is consumed, reducing the number of available buffers by 1
 *
 * @retval pointer to the next available buffer
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
