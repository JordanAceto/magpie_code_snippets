
/* Private includes --------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdint.h>
#include "led.h"
#include "board.h"
#include "mxc_delay.h"

#include "ad4630.h"
#include "audio_dma.h"
#include "sd_card.h"
#include "wav_header.h"

#include <string.h>

/* Private definitions -----------------------------------------------------------------------------------------------*/

// RECORDING TIME IN DMA Blocks (each DMA block is 21.33 ms)
#define RECORDING_TIME_DMA_BLOCKS (1000)

/* Private enumerations ----------------------------------------------------------------------------------------------*/

/**
 * Onboard LED colors for the FTHR2 board are represented here, note that blue and green are reversed wrt original FTHR
 */
typedef enum
{
    LED_COLOR_RED = 0,
    LED_COLOR_GREEN,
    LED_COLOR_BLUE,
} LED_Color_t;

/* Private variables -------------------------------------------------------------------------------------------------*/

// a buffer for writing strings into, for file names and such
static char str_buff[128];

// a variable to store the number of bytes written to the SD card, can be checked against the intended amount
static uint32_t bytes_written;

/* Private function declarations -------------------------------------------------------------------------------------*/

// the error handler simply rapidly blinks the given LED color forever
static void error_handler(LED_Color_t c);

/* Public function definitions ---------------------------------------------------------------------------------------*/

int main(void)
{
    LED_On(LED_COLOR_RED);

    if (sd_card_init() != SD_CARD_ERROR_ALL_OK)
    {
        error_handler(LED_COLOR_RED);
    }

    if (sd_card_mount() != SD_CARD_ERROR_ALL_OK)
    {
        error_handler(LED_COLOR_RED);
    }

    sprintf(str_buff, "test.wav");
    if (sd_card_fopen(str_buff, POSIX_FILE_MODE_WRITE) != SD_CARD_ERROR_ALL_OK)
    {
        error_handler(LED_COLOR_RED);
    }

    // seek past the wave header, we'll fill it in later at the end
    if (sd_card_lseek(wav_header_get_header_length()) != SD_CARD_ERROR_ALL_OK)
    {
        error_handler(LED_COLOR_RED);
    }

    LED_Off(LED_COLOR_RED);

    LED_On(LED_COLOR_BLUE);

    if (ad4630_init() != AD4630_ERROR_ALL_OK)
    {
        error_handler(LED_COLOR_BLUE);
    }
    if (audio_dma_init() != AUDIO_DMA_ERROR_ALL_OK)
    {
        error_handler(LED_COLOR_BLUE);
    }

    ad4630_cont_conversions_start();
    audio_dma_start();

    LED_Off(LED_COLOR_BLUE);

    LED_On(LED_COLOR_GREEN);

    for (uint32_t num_dma_blocks_written = 0; num_dma_blocks_written < RECORDING_TIME_DMA_BLOCKS;)
    {
        if (audio_dma_overrun_occured())
        {
            error_handler(LED_COLOR_BLUE);
        }

        while (audio_dma_num_buffers_available() > 0)
        {
            if (sd_card_fwrite(audio_dma_consume_buffer(), audio_dma_buff_len_in_bytes(), &bytes_written) != SD_CARD_ERROR_ALL_OK)
            {
                error_handler(LED_COLOR_RED);
            }

            num_dma_blocks_written += 1;
        }
    }

    LED_Off(LED_COLOR_GREEN);

    ad4630_cont_conversions_stop();
    audio_dma_stop();

    Wave_Header_Attributes_t wav_attr = {
        .num_channels = WAVE_HEADER_MONO,
        .bits_per_sample = WAVE_HEADER_24_BITS_PER_SAMPLE,
        .sample_rate = WAVE_HEADER_SAMPLE_RATE_384kHz,
        .file_length = sd_card_fsize(),
    };
    wav_header_set_attributes(&wav_attr);

    // back to the top of the file so we can write the wav header
    if (sd_card_lseek(0) != SD_CARD_ERROR_ALL_OK)
    {
        error_handler(LED_COLOR_RED);
    }
    if (sd_card_fwrite(wav_header_get_header(), wav_header_get_header_length(), &bytes_written) != SD_CARD_ERROR_ALL_OK)
    {
        error_handler(LED_COLOR_RED);
    }

    if (sd_card_fclose() != SD_CARD_ERROR_ALL_OK)
    {
        error_handler(LED_COLOR_RED);
    }
    if (sd_card_unmount() != SD_CARD_ERROR_ALL_OK)
    {
        error_handler(LED_COLOR_RED);
    }

    // do a slow green blink to indicate success
    const uint32_t slow_blink = 1000000;
    while (1)
    {
        LED_On(LED_COLOR_GREEN);
        MXC_Delay(slow_blink);
        LED_Off(LED_COLOR_GREEN);
        MXC_Delay(slow_blink);
    }
}

/* Private function definitions --------------------------------------------------------------------------------------*/

void error_handler(LED_Color_t color)
{
    const uint32_t fast_blink = 100000;
    while (true)
    {
        LED_On(color);
        MXC_Delay(fast_blink);
        LED_Off(color);
        MXC_Delay(fast_blink);
    }
}
