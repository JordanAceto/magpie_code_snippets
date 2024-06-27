
/* Private includes --------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdint.h>
#include "led.h"
#include "board.h"
#include "mxc_delay.h"

#include "ad4630.h"
#include "audio_dma.h"
#include "decimation_filter.h"
#include "demo_config.h"
#include "sd_card.h"
#include "wav_header.h"

#include <string.h>

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

// a variable to store the number of bytes written to the SD card, can be checked against the intended amount
static uint32_t bytes_written;

// a buffer for downsampled audio
static uint8_t downsampled_audio[AUDIO_DMA_BUFF_LEN_IN_BYTES];

/* Private function declarations -------------------------------------------------------------------------------------*/

/**
 * @brief `write_demo_wav_file(f, a, l)` writes a wav file with filename `f`, attributes `a`, and length in seconds `l`
 * to the SD card. Calling this function starts the ADC/DMA and continuously records audio in blocking fashion until
 * the time is up.
 *
 * @pre initialization is complete for the ADC, DMA, decimation filters, and SD card, the SD card must be mounted
 *
 * @param fname the name of the file to write
 *
 * @param wav_attr pointer to the wav header attributes structure holding information about sample rate, bit depth, etc
 *
 * @param file_len_secs the length of the audio file to write, in seconds
 *
 * @post this function consumes buffers from the ADC/DMA until the duration of the file length has elapsed and writes
 * the audio data out to a .wav file on the SD card. The wav header for the file is also written in this function.
 */
static void write_demo_wav_file(const char *fname, Wave_Header_Attributes_t *wav_attr, uint32_t file_len_secs);

// the error handler simply rapidly blinks the given LED color forever
static void error_handler(LED_Color_t c);

/* Public function definitions ---------------------------------------------------------------------------------------*/

int main(void)
{
    // blue led on during initialization
    LED_On(LED_COLOR_BLUE);

    decimation_filter_init();

    if (ad4630_init() != AD4630_ERROR_ALL_OK)
    {
        error_handler(LED_COLOR_BLUE);
    }
    if (audio_dma_init() != AUDIO_DMA_ERROR_ALL_OK)
    {
        error_handler(LED_COLOR_BLUE);
    }

    if (sd_card_init() != SD_CARD_ERROR_ALL_OK)
    {
        error_handler(LED_COLOR_RED);
    }

    if (sd_card_mount() != SD_CARD_ERROR_ALL_OK)
    {
        error_handler(LED_COLOR_RED);
    }

    Wave_Header_Attributes_t wav_attr = {
        .num_channels = WAVE_HEADER_MONO, // only mono is supported for now, 2 channel might be added later
    };

    LED_Off(LED_COLOR_BLUE);

    // green led on while writing files
    LED_On(LED_COLOR_GREEN);
    wav_attr.sample_rate = WAVE_HEADER_SAMPLE_RATE_48kHz;
    wav_attr.bits_per_sample = WAVE_HEADER_16_BITS_PER_SAMPLE,
    write_demo_wav_file("demo_48kHz_16_bit.wav", &wav_attr, DEMO_CONFIG_AUDIO_FILE_LEN_IN_SECONDS);
    LED_Off(LED_COLOR_GREEN);

    MXC_Delay(1000000);

    LED_On(LED_COLOR_GREEN);
    wav_attr.sample_rate = WAVE_HEADER_SAMPLE_RATE_96kHz;
    wav_attr.bits_per_sample = WAVE_HEADER_16_BITS_PER_SAMPLE,
    write_demo_wav_file("demo_96kHz_16_bit.wav", &wav_attr, DEMO_CONFIG_AUDIO_FILE_LEN_IN_SECONDS);
    LED_Off(LED_COLOR_GREEN);

    MXC_Delay(1000000);

    LED_On(LED_COLOR_GREEN);
    wav_attr.sample_rate = WAVE_HEADER_SAMPLE_RATE_192kHz;
    wav_attr.bits_per_sample = WAVE_HEADER_16_BITS_PER_SAMPLE,
    write_demo_wav_file("demo_192kHz_16_bit.wav", &wav_attr, DEMO_CONFIG_AUDIO_FILE_LEN_IN_SECONDS);
    LED_Off(LED_COLOR_GREEN);

    MXC_Delay(1000000);

    LED_On(LED_COLOR_GREEN);
    wav_attr.sample_rate = WAVE_HEADER_SAMPLE_RATE_48kHz;
    wav_attr.bits_per_sample = WAVE_HEADER_24_BITS_PER_SAMPLE,
    write_demo_wav_file("demo_48kHz_24_bit.wav", &wav_attr, DEMO_CONFIG_AUDIO_FILE_LEN_IN_SECONDS);
    LED_Off(LED_COLOR_GREEN);

    MXC_Delay(1000000);

    LED_On(LED_COLOR_GREEN);
    wav_attr.sample_rate = WAVE_HEADER_SAMPLE_RATE_96kHz;
    wav_attr.bits_per_sample = WAVE_HEADER_24_BITS_PER_SAMPLE,
    write_demo_wav_file("demo_96kHz_24_bit.wav", &wav_attr, DEMO_CONFIG_AUDIO_FILE_LEN_IN_SECONDS);
    LED_Off(LED_COLOR_GREEN);

    MXC_Delay(1000000);

    LED_On(LED_COLOR_GREEN);
    wav_attr.sample_rate = WAVE_HEADER_SAMPLE_RATE_192kHz;
    wav_attr.bits_per_sample = WAVE_HEADER_24_BITS_PER_SAMPLE,
    write_demo_wav_file("demo_192kHz_24_bit.wav", &wav_attr, DEMO_CONFIG_AUDIO_FILE_LEN_IN_SECONDS);
    LED_Off(LED_COLOR_GREEN);

    MXC_Delay(1000000);

    LED_On(LED_COLOR_GREEN);
    wav_attr.sample_rate = WAVE_HEADER_SAMPLE_RATE_384kHz;
    wav_attr.bits_per_sample = WAVE_HEADER_24_BITS_PER_SAMPLE,
    write_demo_wav_file("demo_384kHz_24_bit.wav", &wav_attr, DEMO_CONFIG_AUDIO_FILE_LEN_IN_SECONDS);
    LED_Off(LED_COLOR_GREEN);

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

void write_demo_wav_file(const char *fname, Wave_Header_Attributes_t *wav_attr, uint32_t file_len_secs)
{
    // there will be some integer truncation here, good enough for this early demo, but improve file-len code eventually
    const uint32_t file_len_in_microsecs = file_len_secs * 1000000;
    const uint32_t num_dma_blocks_in_the_file = file_len_in_microsecs / AUDIO_DMA_CHUNK_READY_PERIOD_IN_MICROSECS;

    if (sd_card_fopen(fname, POSIX_FILE_MODE_WRITE) != SD_CARD_ERROR_ALL_OK)
    {
        error_handler(LED_COLOR_RED);
    }

    // seek past the wave header, we'll fill it in later after recording the audio, we'll know the file length then
    if (sd_card_lseek(wav_header_get_header_length()) != SD_CARD_ERROR_ALL_OK)
    {
        error_handler(LED_COLOR_RED);
    }

    ad4630_cont_conversions_start();
    audio_dma_start();

    for (uint32_t num_dma_blocks_written = 0; num_dma_blocks_written < num_dma_blocks_in_the_file;)
    {
        if (audio_dma_overrun_occured())
        {
            error_handler(LED_COLOR_BLUE);
        }

        while (audio_dma_num_buffers_available() > 0)
        {
            if (wav_attr->sample_rate == WAVE_HEADER_SAMPLE_RATE_384kHz)
            {
                // 384k is a special case, no downsampling here
                if (sd_card_fwrite(audio_dma_consume_buffer(), AUDIO_DMA_BUFF_LEN_IN_BYTES, &bytes_written) != SD_CARD_ERROR_ALL_OK)
                {
                    error_handler(LED_COLOR_RED);
                }
            }
            else // all other sample rates get downsampled before writing to the SD card
            {
                const uint32_t downsampled_buff_len = decimation_filter_downsample(
                    audio_dma_consume_buffer(),
                    AUDIO_DMA_BUFF_LEN_IN_BYTES,
                    downsampled_audio,
                    wav_attr->sample_rate,
                    wav_attr->bits_per_sample);

                if (sd_card_fwrite(downsampled_audio, downsampled_buff_len, &bytes_written) != SD_CARD_ERROR_ALL_OK)
                {
                    error_handler(LED_COLOR_RED);
                }
            }

            num_dma_blocks_written += 1;
        }
    }

    ad4630_cont_conversions_stop();
    audio_dma_stop();

    // back to the top of the file so we can write the wav header now that we can determine the size of the file
    if (sd_card_lseek(0) != SD_CARD_ERROR_ALL_OK)
    {
        error_handler(LED_COLOR_RED);
    }

    wav_attr->file_length = sd_card_fsize();
    wav_header_set_attributes(wav_attr);

    if (sd_card_fwrite(wav_header_get_header(), wav_header_get_header_length(), &bytes_written) != SD_CARD_ERROR_ALL_OK)
    {
        error_handler(LED_COLOR_RED);
    }

    if (sd_card_fclose() != SD_CARD_ERROR_ALL_OK)
    {
        error_handler(LED_COLOR_RED);
    }
}

void error_handler(LED_Color_t color)
{
    LED_Off(LED_COLOR_RED);
    LED_Off(LED_COLOR_GREEN);
    LED_Off(LED_COLOR_BLUE);

    const uint32_t fast_blink = 100000;
    while (true)
    {
        LED_On(color);
        MXC_Delay(fast_blink);
        LED_Off(color);
        MXC_Delay(fast_blink);
    }
}
