
/* Private includes --------------------------------------------------------------------------------------------------*/

#include "arm_fir_decimate_fast_q31_bob.h"
#include "audio_dma.h"
#include "data_converters.h"
#include "decimation_filter.h"
#include "gpio_helpers.h" // used for profiling/timing tests

/* Private defines ---------------------------------------------------------------------------------------------------*/

#define buffLen_deci2x (AUDIO_DMA_BUFF_LEN_IN_SAMPS / 2)   // after 2x decimation 192k mode
#define buffLen_deci4x (AUDIO_DMA_BUFF_LEN_IN_SAMPS / 4)   // after 4x decimation 96k mode
#define buffLen_deci8x (AUDIO_DMA_BUFF_LEN_IN_SAMPS / 8)   // after 8x decimation 48 k mode
#define buffLen_deci12x (AUDIO_DMA_BUFF_LEN_IN_SAMPS / 12) // after 12x decimation, 32 k mode
#define buffLen_deci16x (AUDIO_DMA_BUFF_LEN_IN_SAMPS / 16) // after 16x decimation, 24k mode
#define buffLen_deci24x (AUDIO_DMA_BUFF_LEN_IN_SAMPS / 24) // after 24x decimation, 16 k mode

// 16 k
#define deci_16k_numcoeffs_0 5
#define deci_16k_numcoeffs_1 7
#define deci_16k_numcoeffs_2 9
#define deci_16k_numcoeffs_3 47
#define deci_state_len_16k_0 (AUDIO_DMA_BUFF_LEN_IN_SAMPS + deci_16k_numcoeffs_0 - 1)
#define deci_state_len_16k_1 (buffLen_deci2x + deci_16k_numcoeffs_1 - 1)
#define deci_state_len_16k_2 (buffLen_deci4x + deci_16k_numcoeffs_2 - 1)
#define deci_state_len_16k_3 (buffLen_deci8x + deci_16k_numcoeffs_3 - 1)

// 24 k
#define deci_24k_numcoeffs_0 5
#define deci_24k_numcoeffs_1 7
#define deci_24k_numcoeffs_2 9
#define deci_24k_numcoeffs_3 33
#define deci_state_len_24k_0 (AUDIO_DMA_BUFF_LEN_IN_SAMPS + deci_24k_numcoeffs_0 - 1)
#define deci_state_len_24k_1 (buffLen_deci2x + deci_24k_numcoeffs_1 - 1)
#define deci_state_len_24k_2 (buffLen_deci4x + deci_24k_numcoeffs_2 - 1)
#define deci_state_len_24k_3 (buffLen_deci8x + deci_24k_numcoeffs_3 - 1)

// 32 k
#define deci_32k_numcoeffs_0 7
#define deci_32k_numcoeffs_1 9
#define deci_32k_numcoeffs_2 47
#define deci_state_len_32k_0 (AUDIO_DMA_BUFF_LEN_IN_SAMPS + deci_32k_numcoeffs_0 - 1)
#define deci_state_len_32k_1 (buffLen_deci2x + deci_32k_numcoeffs_1 - 1)
#define deci_state_len_32k_2 (buffLen_deci4x + deci_32k_numcoeffs_2 - 1)

// 48 k
#define deci_48k_numcoeffs_0 7
#define deci_48k_numcoeffs_1 9
#define deci_48k_numcoeffs_2 33
#define deci_state_len_48k_0 (AUDIO_DMA_BUFF_LEN_IN_SAMPS + deci_48k_numcoeffs_0 - 1)
#define deci_state_len_48k_1 (buffLen_deci2x + deci_48k_numcoeffs_1 - 1)
#define deci_state_len_48k_2 (buffLen_deci4x + deci_48k_numcoeffs_2 - 1)

// 96 k
#define deci_96k_numcoeffs_0 9
#define deci_96k_numcoeffs_1 33
#define deci_state_len_96k_0 (AUDIO_DMA_BUFF_LEN_IN_SAMPS + deci_96k_numcoeffs_0 - 1)
#define deci_state_len_96k_1 (buffLen_deci2x + deci_96k_numcoeffs_1 - 1)

// 192 k
#define deci_192k_numcoeffs_0 17
#define deci_state_len_192k_0 (AUDIO_DMA_BUFF_LEN_IN_SAMPS + deci_192k_numcoeffs_0 - 1)

/* Private variables -------------------------------------------------------------------------------------------------*/

// 16 k
static const q31_t firCoeffs_16k_0[deci_16k_numcoeffs_0] = {
    86385383, 380767973, 588547483, 380767973, 86385383};
static const q31_t firCoeffs_16k_1[deci_16k_numcoeffs_1] = {
    -63623254, 11777617, 601023485, 1051356664, 601023485, 11777617, -63623254};
static const q31_t firCoeffs_16k_2[deci_16k_numcoeffs_2] = {
    -27728029, -77658950, 90916825, 613537738, 945568961, 613537738, 90916825, -77658950, -27728029};
static const q31_t firCoeffs_16k_3[deci_16k_numcoeffs_3] = {
    544679, 5591621, 10519170, 11396576, 3740919, -9199321, -16459240, -8186184, 12423382, 27105429, 17140829, -15659956, -43184586, -32667249, 18552286, 68866053, 61256147, -20863109, -119392334, -129148756, 22338651, 303309288, 578589578, 692986716, 578589578, 303309288, 22338651, -129148756, -119392334, -20863109, 61256147, 68866053, 18552286, -32667249, -43184586, -15659956, 17140829, 27105429, 12423382, -8186184, -16459240, -9199321, 3740919, 11396576, 10519170, 5591621, 544679};

// 24k
static const q31_t firCoeffs_24k_0[deci_24k_numcoeffs_0] = {
    87026071, 382177371, 589816446, 382177371, 87026071};
static const q31_t firCoeffs_24k_1[deci_24k_numcoeffs_1] = {
    -59682168, 25489114, 599055019, 1028294198, 599055019, 25489114, -59682168};
static const q31_t firCoeffs_24k_2[deci_24k_numcoeffs_2] = {
    -35829136, -93392547, 90204797, 624894336, 955274946, 624894336, 90204797, -93392547, -35829136};
static const q31_t firCoeffs_24k_3[deci_24k_numcoeffs_3] = {
    -2823963, 804105, 13756249, 13832557, -12099816, -21810016, 17681236, 39284877, -22118934, -66381589, 26258540, 112809109, -29540929, -212849373, 31655722, 678451831, 1041361918, 678451831, 31655722, -212849373, -29540929, 112809109, 26258540, -66381589, -22118934, 39284877, 17681236, -21810016, -12099816, 13832557, 13756249, 804105, -2823963};

// 32 k
static const q31_t firCoeffs_32k_0[deci_32k_numcoeffs_0] = {
    -44988434, 8328033, 424987782, 743421426, 424987782, 8328033, -44988434};
static const q31_t firCoeffs_32k_1[deci_32k_numcoeffs_1] = {
    -27728029, -77658950, 90916825, 613537738, 945568961, 613537738, 90916825, -77658950, -27728029};
static const q31_t firCoeffs_32k_2[deci_32k_numcoeffs_2] = {
    544679, 5591621, 10519170, 11396576, 3740919, -9199321, -16459240, -8186184, 12423382, 27105429, 17140829, -15659956, -43184586, -32667249, 18552286, 68866053, 61256147, -20863109, -119392334, -129148756, 22338651, 303309288, 578589578, 692986716, 578589578, 303309288, 22338651, -129148756, -119392334, -20863109, 61256147, 68866053, 18552286, -32667249, -43184586, -15659956, 17140829, 27105429, 12423382, -8186184, -16459240, -9199321, 3740919, 11396576, 10519170, 5591621, 544679};

// 48 k
static const q31_t firCoeffs_48k_0[deci_48k_numcoeffs_0] = {
    -42201666, 18023525, 423595866, 727113801, 423595866, 18023525, -42201666};
static const q31_t firCoeffs_48k_1[deci_48k_numcoeffs_1] = {
    -35829136, -93392547, 90204797, 624894336, 955274946, 624894336, 90204797, -93392547, -35829136};
static const q31_t firCoeffs_48k_2[deci_48k_numcoeffs_2] = {
    -2823963, 804105, 13756249, 13832557, -12099816, -21810016, 17681236, 39284877, -22118934, -66381589, 26258540, 112809109, -29540929, -212849373, 31655722, 678451831, 1041361918, 678451831, 31655722, -212849373, -29540929, 112809109, 26258540, -66381589, -22118934, 39284877, 17681236, -21810016, -12099816, 13832557, 13756249, 804105, -2823963};

// 96 k
static const q31_t firCoeffs_96k_0[deci_96k_numcoeffs_0] = {
    -20749647, -66609278, 51582801, 442242045, 691682165, 442242045, 51582801, -66609278, -20749647};
static const q31_t firCoeffs_96k_1[deci_96k_numcoeffs_1] = {
    -3229201, 1658598, 16721610, 16330065, -12855261, -23661054, 18450717, 41258441, -22628821, -68277309, 26456118, 114386501, -29451143, -213892037, 31368109, 678814008, 1041713732, 678814008, 31368109, -213892037, -29451143, 114386501, 26456118, -68277309, -22628821, 41258441, 18450717, -23661054, -12855261, 16330065, 16721610, 1658598, -3229201};

// 192 k
static const q31_t firCoeffs_192k_0[deci_192k_numcoeffs_0] = {
    -21932835, -31283673, 20226235, 65093442, -23389170, -137747312, 30271796, 477147941, 730493753, 477147941, 30271796, -137747312, -23389170, 65093442, 20226235, -31283673, -21932835};

// decimated buffers for various stages of the multi-rate filters, reused in a ping-pong fashion to save SRAM
static q31_t rx_ping_pong_buff_0[buffLen_deci2x]; // guaranteed to be big enough for the first round of any sample rate
static q31_t rx_ping_pong_buff_1[buffLen_deci4x]; // guaranteed to be big enough for the second round of any sample rate

static q31_t fir_state_pin_pong_0[deci_state_len_192k_0]; // guaranteed to be big enough for the first round of any sample rate
static q31_t fir_state_pin_pong_1[deci_state_len_96k_1];  // guaranteed to be big enough for the second round of any sample rate

// 16 k
arm_fir_decimate_instance_q31 Sdeci_16k_0;
arm_fir_decimate_instance_q31 Sdeci_16k_1;
arm_fir_decimate_instance_q31 Sdeci_16k_2;
arm_fir_decimate_instance_q31 Sdeci_16k_3;

// 24 k
arm_fir_decimate_instance_q31 Sdeci_24k_0;
arm_fir_decimate_instance_q31 Sdeci_24k_1;
arm_fir_decimate_instance_q31 Sdeci_24k_2;
arm_fir_decimate_instance_q31 Sdeci_24k_3;

// 32 k
arm_fir_decimate_instance_q31 Sdeci_32k_0;
arm_fir_decimate_instance_q31 Sdeci_32k_1;
arm_fir_decimate_instance_q31 Sdeci_32k_2;

// 48 k
arm_fir_decimate_instance_q31 Sdeci_48k_0;
arm_fir_decimate_instance_q31 Sdeci_48k_1;
arm_fir_decimate_instance_q31 Sdeci_48k_2;

// 96 k
arm_fir_decimate_instance_q31 Sdeci_96k_0;
arm_fir_decimate_instance_q31 Sdeci_96k_1;

// 192 k
arm_fir_decimate_instance_q31 Sdeci_192k_0;

/* Private function declarations -------------------------------------------------------------------------------------*/

/**
 * @brief `truncate_bit_depth_from_q31(s, d, sl, bd)` truncates the 32 bit samples in buffer `s` to either 16 or 24 bits
 * depending on bit depth `bd` and stores them in destination buffer `d`
 *
 * @param src the source buffer to truncate
 *
 * @param dest the destination for the truncated samples
 *
 * @param src_len_in_samps the number of samples to transfer, must obey the following preconditions:
 * if bit_depth is 16 bits: can be any length,
 * if bit_depth is 24 bits: must be a multiple of 4
 *
 * @param bit_depth the enumerated bit depth to truncate to, either 16 or 24 bits
 *
 * @post the samples from `s` are truncated and stored in `d`
 */
uint32_t truncate_bit_depth_from_q31(q31_t *src, uint8_t *dest, uint32_t src_len_in_samps, Wave_Header_Bits_Per_Sample_t bit_depth);

/* Public function definitions ---------------------------------------------------------------------------------------*/

void decimation_filter_init()
{
    arm_fir_decimate_init_q31(&Sdeci_16k_0, deci_16k_numcoeffs_0, 2, &firCoeffs_16k_0[0], &fir_state_pin_pong_0[0], AUDIO_DMA_BUFF_LEN_IN_SAMPS);
    arm_fir_decimate_init_q31(&Sdeci_16k_1, deci_16k_numcoeffs_1, 2, &firCoeffs_16k_1[0], &fir_state_pin_pong_1[0], buffLen_deci2x);
    arm_fir_decimate_init_q31(&Sdeci_16k_2, deci_16k_numcoeffs_2, 2, &firCoeffs_16k_2[0], &fir_state_pin_pong_0[0], buffLen_deci4x);
    arm_fir_decimate_init_q31(&Sdeci_16k_3, deci_16k_numcoeffs_3, 3, &firCoeffs_16k_3[0], &fir_state_pin_pong_1[0], buffLen_deci8x);

    arm_fir_decimate_init_q31(&Sdeci_24k_0, deci_24k_numcoeffs_0, 2, &firCoeffs_24k_0[0], &fir_state_pin_pong_0[0], AUDIO_DMA_BUFF_LEN_IN_SAMPS);
    arm_fir_decimate_init_q31(&Sdeci_24k_1, deci_24k_numcoeffs_1, 2, &firCoeffs_24k_1[0], &fir_state_pin_pong_1[0], buffLen_deci2x);
    arm_fir_decimate_init_q31(&Sdeci_24k_2, deci_24k_numcoeffs_2, 2, &firCoeffs_24k_2[0], &fir_state_pin_pong_0[0], buffLen_deci4x);
    arm_fir_decimate_init_q31(&Sdeci_24k_3, deci_24k_numcoeffs_3, 2, &firCoeffs_24k_3[0], &fir_state_pin_pong_1[0], buffLen_deci8x);

    arm_fir_decimate_init_q31(&Sdeci_32k_0, deci_32k_numcoeffs_0, 2, &firCoeffs_32k_0[0], &fir_state_pin_pong_0[0], AUDIO_DMA_BUFF_LEN_IN_SAMPS);
    arm_fir_decimate_init_q31(&Sdeci_32k_1, deci_32k_numcoeffs_1, 2, &firCoeffs_32k_1[0], &fir_state_pin_pong_1[0], buffLen_deci2x);
    arm_fir_decimate_init_q31(&Sdeci_32k_2, deci_32k_numcoeffs_2, 3, &firCoeffs_32k_2[0], &fir_state_pin_pong_0[0], buffLen_deci4x);

    arm_fir_decimate_init_q31(&Sdeci_48k_0, deci_48k_numcoeffs_0, 2, &firCoeffs_48k_0[0], &fir_state_pin_pong_0[0], AUDIO_DMA_BUFF_LEN_IN_SAMPS);
    arm_fir_decimate_init_q31(&Sdeci_48k_1, deci_48k_numcoeffs_1, 2, &firCoeffs_48k_1[0], &fir_state_pin_pong_1[0], buffLen_deci2x);
    arm_fir_decimate_init_q31(&Sdeci_48k_2, deci_48k_numcoeffs_2, 2, &firCoeffs_48k_2[0], &fir_state_pin_pong_0[0], buffLen_deci4x);

    arm_fir_decimate_init_q31(&Sdeci_96k_0, deci_96k_numcoeffs_0, 2, &firCoeffs_96k_0[0], &fir_state_pin_pong_0[0], AUDIO_DMA_BUFF_LEN_IN_SAMPS);
    arm_fir_decimate_init_q31(&Sdeci_96k_1, deci_96k_numcoeffs_1, 2, &firCoeffs_96k_1[0], &fir_state_pin_pong_1[0], buffLen_deci2x);

    arm_fir_decimate_init_q31(&Sdeci_192k_0, deci_192k_numcoeffs_0, 2, &firCoeffs_192k_0[0], &fir_state_pin_pong_0[0], AUDIO_DMA_BUFF_LEN_IN_SAMPS);
}

uint32_t decimation_filter_downsample(
    uint8_t *src_384kHz,
    uint32_t src_len_in_bytes,
    uint8_t *dest,
    Wave_Header_Sample_Rate_t dest_sample_rate,
    Wave_Header_Bits_Per_Sample_t dest_bit_depth)
{
    q31_t *filtered_q31s;
    uint32_t filtered_buff_len;

    switch (dest_sample_rate)
    {
    case WAVE_HEADER_SAMPLE_RATE_384kHz:
        // 384k is a special case, no filtering, just optional truncation to 16 bits, early return for this case
        if (dest_bit_depth == WAVE_HEADER_24_BITS_PER_SAMPLE)
        {
            // TODO: profile this naive loop, if it takes too long consider replacing it with a partially unrolled loop
            // or even just a pointer swap (this would require making dest a pointer-to-a-pointer)
            for (uint32_t i = 0; i < src_len_in_bytes; i++)
            {
                dest[i] = src_384kHz[i];
            }


            return src_len_in_bytes;
        }
        else // it must be 16 bit samples
        {
            data_converters_i24_to_q15(src_384kHz, dest, src_len_in_bytes);
            return (src_len_in_bytes * DATA_CONVERTERS_Q15_SIZE_IN_BYTES) / DATA_CONVERTERS_I24_SIZE_IN_BYTES;
        }

    case WAVE_HEADER_SAMPLE_RATE_192kHz:
        arm_fir_decimate_fast_q31_bob(&Sdeci_192k_0, src_384kHz, rx_ping_pong_buff_0, AUDIO_DMA_BUFF_LEN_IN_SAMPS);

        filtered_q31s = rx_ping_pong_buff_0;
        filtered_buff_len = buffLen_deci2x;

        break;

    case WAVE_HEADER_SAMPLE_RATE_96kHz:
        arm_fir_decimate_fast_q31_bob(&Sdeci_96k_0, src_384kHz, rx_ping_pong_buff_0, AUDIO_DMA_BUFF_LEN_IN_SAMPS);
        arm_fir_decimate_fast_q31_bob(&Sdeci_96k_1, rx_ping_pong_buff_0, rx_ping_pong_buff_1, buffLen_deci2x);

        filtered_q31s = rx_ping_pong_buff_1;
        filtered_buff_len = buffLen_deci4x;

        break;

    case WAVE_HEADER_SAMPLE_RATE_48kHz:
        arm_fir_decimate_fast_q31_bob(&Sdeci_48k_0, src_384kHz, rx_ping_pong_buff_0, AUDIO_DMA_BUFF_LEN_IN_SAMPS);
        arm_fir_decimate_fast_q31_bob(&Sdeci_48k_1, rx_ping_pong_buff_0, rx_ping_pong_buff_1, buffLen_deci2x);
        arm_fir_decimate_fast_q31_bob(&Sdeci_48k_2, rx_ping_pong_buff_1, rx_ping_pong_buff_0, buffLen_deci4x);

        filtered_q31s = rx_ping_pong_buff_0;
        filtered_buff_len = buffLen_deci8x;

        break;

    case WAVE_HEADER_SAMPLE_RATE_32kHz:
        arm_fir_decimate_fast_q31_bob(&Sdeci_32k_0, src_384kHz, rx_ping_pong_buff_0, AUDIO_DMA_BUFF_LEN_IN_SAMPS);
        arm_fir_decimate_fast_q31_bob(&Sdeci_32k_1, rx_ping_pong_buff_0, rx_ping_pong_buff_1, buffLen_deci2x);
        arm_fir_decimate_fast_q31_bob(&Sdeci_32k_2, rx_ping_pong_buff_1, rx_ping_pong_buff_0, buffLen_deci4x);

        filtered_q31s = rx_ping_pong_buff_0;
        filtered_buff_len = buffLen_deci12x;

        break;

    case WAVE_HEADER_SAMPLE_RATE_24kHz:
        arm_fir_decimate_fast_q31_bob(&Sdeci_24k_0, src_384kHz, rx_ping_pong_buff_0, AUDIO_DMA_BUFF_LEN_IN_SAMPS);
        arm_fir_decimate_fast_q31_bob(&Sdeci_24k_1, rx_ping_pong_buff_0, rx_ping_pong_buff_1, buffLen_deci2x);
        arm_fir_decimate_fast_q31_bob(&Sdeci_24k_2, rx_ping_pong_buff_1, rx_ping_pong_buff_0, buffLen_deci4x);
        arm_fir_decimate_fast_q31_bob(&Sdeci_24k_3, rx_ping_pong_buff_0, rx_ping_pong_buff_1, buffLen_deci8x);

        filtered_q31s = rx_ping_pong_buff_1;
        filtered_buff_len = buffLen_deci16x;

        break;

    case WAVE_HEADER_SAMPLE_RATE_16kHz:
        arm_fir_decimate_fast_q31_bob(&Sdeci_16k_0, src_384kHz, rx_ping_pong_buff_0, AUDIO_DMA_BUFF_LEN_IN_SAMPS);
        arm_fir_decimate_fast_q31_bob(&Sdeci_16k_1, rx_ping_pong_buff_0, rx_ping_pong_buff_1, buffLen_deci2x);
        arm_fir_decimate_fast_q31_bob(&Sdeci_16k_2, rx_ping_pong_buff_1, rx_ping_pong_buff_0, buffLen_deci4x);
        arm_fir_decimate_fast_q31_bob(&Sdeci_16k_3, rx_ping_pong_buff_0, rx_ping_pong_buff_1, buffLen_deci8x);

        filtered_q31s = rx_ping_pong_buff_1;
        filtered_buff_len = buffLen_deci24x;

        break;
    }

    const uint32_t retval = truncate_bit_depth_from_q31(filtered_q31s, dest, filtered_buff_len, dest_bit_depth);

    return retval;
}

/* Private function definitions --------------------------------------------------------------------------------------*/

uint32_t truncate_bit_depth_from_q31(q31_t *src, uint8_t *dest, uint32_t src_len_in_samps, Wave_Header_Bits_Per_Sample_t bit_depth)
{
    if (bit_depth == WAVE_HEADER_16_BITS_PER_SAMPLE)
    {
        arm_q31_to_q15(src, (q15_t *)dest, src_len_in_samps);
        return src_len_in_samps * DATA_CONVERTERS_Q15_SIZE_IN_BYTES;
    }
    else // it must by 24 bit
    {
        data_converters_q31_to_i24(src, dest, src_len_in_samps);
        return src_len_in_samps * DATA_CONVERTERS_I24_SIZE_IN_BYTES;
    }
}
