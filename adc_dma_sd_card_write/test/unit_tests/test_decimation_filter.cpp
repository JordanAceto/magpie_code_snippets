#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_helpers.hpp"

extern "C"
{
#include "audio_dma.h"
#include "decimation_filter.h"
}

using namespace testing;

TEST(DecimationFilterTest, all_sample_rates_yield_correct_buff_lengths_as_output)
{
    const auto sample_rates = std::array<Wave_Header_Sample_Rate_t, 7>{
        WAVE_HEADER_SAMPLE_RATE_384kHz,
        WAVE_HEADER_SAMPLE_RATE_192kHz,
        WAVE_HEADER_SAMPLE_RATE_96kHz,
        WAVE_HEADER_SAMPLE_RATE_48kHz,
        WAVE_HEADER_SAMPLE_RATE_32kHz,
        WAVE_HEADER_SAMPLE_RATE_24kHz,
        WAVE_HEADER_SAMPLE_RATE_16kHz,
    };

    const auto bit_depths = std::array<Wave_Header_Bits_Per_Sample_t, 2>{
        WAVE_HEADER_24_BITS_PER_SAMPLE,
        WAVE_HEADER_16_BITS_PER_SAMPLE,
    };

    uint8_t src[AUDIO_DMA_LARGEST_BUFF_LEN_IN_BYTES];
    uint8_t dest[AUDIO_DMA_LARGEST_BUFF_LEN_IN_BYTES];

    for (const auto sr : sample_rates)
    {
        for (const auto bd : bit_depths)
        {
            decimation_filter_set_sample_rate(sr);
            const uint32_t actual_len = decimation_filter_downsample(src, AUDIO_DMA_BUFF_LEN_IN_SAMPS * 3, dest, bd);

            const uint32_t decimation_factor = WAVE_HEADER_SAMPLE_RATE_384kHz / sr;
            const uint32_t samp_width_in_bytes = bd / 8;

            const uint32_t expected_len = (AUDIO_DMA_BUFF_LEN_IN_SAMPS / decimation_factor) * samp_width_in_bytes;

            ASSERT_EQ(actual_len, expected_len);
        }
    }
}

TEST(DecimationFilterTest, sr_384k_24b_should_not_change_anything)
{
    uint8_t src[AUDIO_DMA_LARGEST_BUFF_LEN_IN_BYTES];
    uint8_t dest[AUDIO_DMA_LARGEST_BUFF_LEN_IN_BYTES];

    // arbitrary seed so we can reproduce if needed
    std::srand(12345);

    // fill src with arbitrary non-zero data
    for (int i = 0; i < AUDIO_DMA_LARGEST_BUFF_LEN_IN_BYTES; i++)
    {
        src[i] = std::rand();
    }

    decimation_filter_set_sample_rate(WAVE_HEADER_SAMPLE_RATE_384kHz);

    decimation_filter_downsample(src, AUDIO_DMA_LARGEST_BUFF_LEN_IN_BYTES, dest, WAVE_HEADER_24_BITS_PER_SAMPLE);

    ASSERT_THAT(src, ContainerEq(dest));
}

TEST(DecimationFilterTest, sr_384k_16b_truncates_3_byte_samples_to_2)
{
    uint32_t src_len_in_bytes = 12;
    uint8_t src[12] = {
        0x00, 0x11, 0x22, // 1st sample
        0x33, 0x44, 0x55,
        0x66, 0x77, 0x88,
        0x99, 0xAA, 0xBB}; // 4th sample

    uint8_t dest[8] = {0}; // 12 samples from src * 2/3 = 8 bytes in dest

    decimation_filter_set_sample_rate(WAVE_HEADER_SAMPLE_RATE_384kHz);

    decimation_filter_downsample(src, src_len_in_bytes, dest, WAVE_HEADER_16_BITS_PER_SAMPLE);

    ASSERT_THAT(dest, ElementsAre(
                          0x11, 0x22, // 1st sample, ls byte gone
                          0x44, 0x55,
                          0x77, 0x88,
                          0xAA, 0xBB));
}

TEST(DecimationFilterTest, changing_sample_rates_does_not_influence_future_calls)
{
    uint8_t src[AUDIO_DMA_LARGEST_BUFF_LEN_IN_BYTES] = {0};

    uint8_t dest_0[AUDIO_DMA_LARGEST_BUFF_LEN_IN_BYTES] = {0};
    uint8_t dest_1[AUDIO_DMA_LARGEST_BUFF_LEN_IN_BYTES] = {0};

    // fill src with arbitrary non-zero data
    for (int i = 0; i < AUDIO_DMA_LARGEST_BUFF_LEN_IN_BYTES; i++)
    {
        src[i] = i % 1000;
    }

    decimation_filter_set_sample_rate(WAVE_HEADER_SAMPLE_RATE_192kHz);

    // store filtered samples in dest_0, we'll compare this against dest_1 later
    decimation_filter_downsample(src, AUDIO_DMA_LARGEST_BUFF_LEN_IN_BYTES, dest_0, WAVE_HEADER_24_BITS_PER_SAMPLE);

    decimation_filter_set_sample_rate(WAVE_HEADER_SAMPLE_RATE_16kHz);

    // store different filtered samples in dest_1, we use a different sample rate and bit depth here
    decimation_filter_downsample(src, AUDIO_DMA_LARGEST_BUFF_LEN_IN_BYTES, dest_1, WAVE_HEADER_16_BITS_PER_SAMPLE);

    // setting the sample rate back to 192k resets any impact that the 16k call above had
    decimation_filter_set_sample_rate(WAVE_HEADER_SAMPLE_RATE_192kHz);

    // after this dest_1 should be identical to dest_0, which shows that the middle call to decimate at 16k did not influence the final call to 192k
    decimation_filter_downsample(src, AUDIO_DMA_LARGEST_BUFF_LEN_IN_BYTES, dest_1, WAVE_HEADER_24_BITS_PER_SAMPLE);

    ASSERT_THAT(dest_0, ContainerEq(dest_1));
}
