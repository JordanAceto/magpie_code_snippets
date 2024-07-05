
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

static q31_t dmaDestBuff_32bit[AUDIO_DMA_BUFF_LEN_IN_SAMPS] = {0}; // same data but assembled back into 32-bit words

// static uint8_t *output_buffer[AUDIO_DMA_BUFF_LEN_IN_BYTES];

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

// decimated buffers for various stages of the multi-rate filters
static q31_t dma_rx_deci_2x[buffLen_deci2x] = {0};   // 1st 2X decimator output -> 192k
static q31_t dma_rx_deci_4x[buffLen_deci4x] = {0};   // 2nd 2X decimator -> 96k
static q31_t dma_rx_deci_8x[buffLen_deci8x] = {0};   // 3rd 2X decimator -> 48k
static q31_t dma_rx_deci_12x[buffLen_deci12x] = {0}; // 3rd 3X decimator, 32k mode
static q31_t dma_rx_deci_16x[buffLen_deci16x] = {0}; // 4th 2X decimator -> 24k
static q31_t dma_rx_deci_24x[buffLen_deci24x] = {0}; // 4th 3X decimator, 16k mode

// 16 k
static q31_t firState_16k_0[deci_state_len_16k_0] = {0};
static q31_t firState_16k_1[deci_state_len_16k_1] = {0};
static q31_t firState_16k_2[deci_state_len_16k_2] = {0};
static q31_t firState_16k_3[deci_state_len_16k_3] = {0};

// 24 k
static q31_t firState_24k_0[deci_state_len_24k_0] = {0};
static q31_t firState_24k_1[deci_state_len_24k_1] = {0};
static q31_t firState_24k_2[deci_state_len_24k_2] = {0};
static q31_t firState_24k_3[deci_state_len_24k_3] = {0};

// 32 k
static q31_t firState_32k_0[deci_state_len_32k_0] = {0};
static q31_t firState_32k_1[deci_state_len_32k_1] = {0};
static q31_t firState_32k_2[deci_state_len_32k_2] = {0};

// 48 k
static q31_t firState_48k_0[deci_state_len_48k_0] = {0};
static q31_t firState_48k_1[deci_state_len_48k_1] = {0};
static q31_t firState_48k_2[deci_state_len_48k_2] = {0};

// 96 k
static q31_t firState_96k_0[deci_state_len_96k_0] = {0};
static q31_t firState_96k_1[deci_state_len_96k_1] = {0};

// 192 k
static q31_t firState_192k_0[deci_state_len_192k_0] = {0};

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

uint32_t truncate_bit_depth(q31_t *src, uint8_t *dest, uint32_t src_len_in_samps, Wave_Header_Bits_Per_Sample_t bit_depth);

/* Public function definitions ---------------------------------------------------------------------------------------*/

void decimation_filter_init()
{
    arm_fir_decimate_init_q31(&Sdeci_16k_0, deci_16k_numcoeffs_0, 2, &firCoeffs_16k_0[0], &firState_16k_0[0], AUDIO_DMA_BUFF_LEN_IN_SAMPS);
    arm_fir_decimate_init_q31(&Sdeci_16k_1, deci_16k_numcoeffs_1, 2, &firCoeffs_16k_1[0], &firState_16k_1[0], buffLen_deci2x);
    arm_fir_decimate_init_q31(&Sdeci_16k_2, deci_16k_numcoeffs_2, 2, &firCoeffs_16k_2[0], &firState_16k_2[0], buffLen_deci4x);
    arm_fir_decimate_init_q31(&Sdeci_16k_3, deci_16k_numcoeffs_3, 3, &firCoeffs_16k_3[0], &firState_16k_3[0], buffLen_deci8x);

    arm_fir_decimate_init_q31(&Sdeci_24k_0, deci_24k_numcoeffs_0, 2, &firCoeffs_24k_0[0], &firState_24k_0[0], AUDIO_DMA_BUFF_LEN_IN_SAMPS);
    arm_fir_decimate_init_q31(&Sdeci_24k_1, deci_24k_numcoeffs_1, 2, &firCoeffs_24k_1[0], &firState_24k_1[0], buffLen_deci2x);
    arm_fir_decimate_init_q31(&Sdeci_24k_2, deci_24k_numcoeffs_2, 2, &firCoeffs_24k_2[0], &firState_24k_2[0], buffLen_deci4x);
    arm_fir_decimate_init_q31(&Sdeci_24k_3, deci_24k_numcoeffs_3, 2, &firCoeffs_24k_3[0], &firState_24k_3[0], buffLen_deci8x);

    arm_fir_decimate_init_q31(&Sdeci_32k_0, deci_32k_numcoeffs_0, 2, &firCoeffs_32k_0[0], &firState_32k_0[0], AUDIO_DMA_BUFF_LEN_IN_SAMPS);
    arm_fir_decimate_init_q31(&Sdeci_32k_1, deci_32k_numcoeffs_1, 2, &firCoeffs_32k_1[0], &firState_32k_1[0], buffLen_deci2x);
    arm_fir_decimate_init_q31(&Sdeci_32k_2, deci_32k_numcoeffs_2, 3, &firCoeffs_32k_2[0], &firState_32k_2[0], buffLen_deci4x);

    arm_fir_decimate_init_q31(&Sdeci_48k_0, deci_48k_numcoeffs_0, 2, &firCoeffs_48k_0[0], &firState_48k_0[0], AUDIO_DMA_BUFF_LEN_IN_SAMPS);
    arm_fir_decimate_init_q31(&Sdeci_48k_1, deci_48k_numcoeffs_1, 2, &firCoeffs_48k_1[0], &firState_48k_1[0], buffLen_deci2x);
    arm_fir_decimate_init_q31(&Sdeci_48k_2, deci_48k_numcoeffs_2, 2, &firCoeffs_48k_2[0], &firState_48k_2[0], buffLen_deci4x);

    arm_fir_decimate_init_q31(&Sdeci_96k_0, deci_96k_numcoeffs_0, 2, &firCoeffs_96k_0[0], &firState_96k_0[0], AUDIO_DMA_BUFF_LEN_IN_SAMPS);
    arm_fir_decimate_init_q31(&Sdeci_96k_1, deci_96k_numcoeffs_1, 2, &firCoeffs_96k_1[0], &firState_96k_1[0], buffLen_deci2x);

    arm_fir_decimate_init_q31(&Sdeci_192k_0, deci_192k_numcoeffs_0, 2, &firCoeffs_192k_0[0], &firState_192k_0[0], AUDIO_DMA_BUFF_LEN_IN_SAMPS);
}

uint32_t decimation_filter_downsample(
    uint8_t *src_384kHz_24_bit,
    uint32_t src_len_in_bytes,
    uint8_t *dest,
    Wave_Header_Sample_Rate_t dest_sample_rate,
    Wave_Header_Bits_Per_Sample_t dest_bit_depth)
{
    data_converters_i24_to_q31(src_384kHz_24_bit, dmaDestBuff_32bit, src_len_in_bytes);

    q31_t *src;
    uint32_t buff_len;

    switch (dest_sample_rate)
    {
    case WAVE_HEADER_SAMPLE_RATE_192kHz:
        arm_fir_decimate_fast_q31_bob(&Sdeci_192k_0, dmaDestBuff_32bit, dma_rx_deci_2x, AUDIO_DMA_BUFF_LEN_IN_SAMPS);

        src = dma_rx_deci_2x;
        buff_len = buffLen_deci2x;

        break;

    case WAVE_HEADER_SAMPLE_RATE_96kHz:
        arm_fir_decimate_fast_q31_bob(&Sdeci_96k_0, dmaDestBuff_32bit, dma_rx_deci_2x, AUDIO_DMA_BUFF_LEN_IN_SAMPS);
        arm_fir_decimate_fast_q31_bob(&Sdeci_96k_1, dma_rx_deci_2x, dma_rx_deci_4x, buffLen_deci2x);

        src = dma_rx_deci_4x;
        buff_len = buffLen_deci4x;

        break;

    case WAVE_HEADER_SAMPLE_RATE_48kHz:
        arm_fir_decimate_fast_q31_bob(&Sdeci_48k_0, dmaDestBuff_32bit, dma_rx_deci_2x, AUDIO_DMA_BUFF_LEN_IN_SAMPS);
        arm_fir_decimate_fast_q31_bob(&Sdeci_48k_1, dma_rx_deci_2x, dma_rx_deci_4x, buffLen_deci2x);
        arm_fir_decimate_fast_q31_bob(&Sdeci_48k_2, dma_rx_deci_4x, dma_rx_deci_8x, buffLen_deci4x);

        src = dma_rx_deci_8x;
        buff_len = buffLen_deci8x;

        break;

    case WAVE_HEADER_SAMPLE_RATE_32kHz:
        arm_fir_decimate_fast_q31_bob(&Sdeci_32k_0, dmaDestBuff_32bit, dma_rx_deci_2x, AUDIO_DMA_BUFF_LEN_IN_SAMPS);
        arm_fir_decimate_fast_q31_bob(&Sdeci_32k_1, dma_rx_deci_2x, dma_rx_deci_4x, buffLen_deci2x);
        arm_fir_decimate_fast_q31_bob(&Sdeci_32k_2, dma_rx_deci_4x, dma_rx_deci_12x, buffLen_deci4x);

        src = dma_rx_deci_12x;
        buff_len = buffLen_deci12x;

        break;

    case WAVE_HEADER_SAMPLE_RATE_24kHz:
        arm_fir_decimate_fast_q31_bob(&Sdeci_24k_0, dmaDestBuff_32bit, dma_rx_deci_2x, AUDIO_DMA_BUFF_LEN_IN_SAMPS);
        arm_fir_decimate_fast_q31_bob(&Sdeci_24k_1, dma_rx_deci_2x, dma_rx_deci_4x, buffLen_deci2x);
        arm_fir_decimate_fast_q31_bob(&Sdeci_24k_2, dma_rx_deci_4x, dma_rx_deci_8x, buffLen_deci4x);
        arm_fir_decimate_fast_q31_bob(&Sdeci_24k_3, dma_rx_deci_8x, dma_rx_deci_16x, buffLen_deci8x);

        src = dma_rx_deci_16x;
        buff_len = buffLen_deci16x;

        break;

    case WAVE_HEADER_SAMPLE_RATE_16kHz:
        arm_fir_decimate_fast_q31_bob(&Sdeci_16k_0, dmaDestBuff_32bit, dma_rx_deci_2x, AUDIO_DMA_BUFF_LEN_IN_SAMPS);
        arm_fir_decimate_fast_q31_bob(&Sdeci_16k_1, dma_rx_deci_2x, dma_rx_deci_4x, buffLen_deci2x);
        arm_fir_decimate_fast_q31_bob(&Sdeci_16k_2, dma_rx_deci_4x, dma_rx_deci_8x, buffLen_deci4x);
        arm_fir_decimate_fast_q31_bob(&Sdeci_16k_3, dma_rx_deci_8x, dma_rx_deci_24x, buffLen_deci8x);

        src = dma_rx_deci_24x;
        buff_len = buffLen_deci24x;

        break;

    default:
        // TODO: handle the 384kHz case
        return 0;
    }

    const uint32_t retval = truncate_bit_depth(src, dest, buff_len, dest_bit_depth);

    return retval;
}

/* Private function definitions --------------------------------------------------------------------------------------*/

uint32_t truncate_bit_depth(q31_t *src, uint8_t *dest, uint32_t src_len_in_samps, Wave_Header_Bits_Per_Sample_t bit_depth)
{
    if (bit_depth == WAVE_HEADER_16_BITS_PER_SAMPLE)
    {
        arm_q31_to_q15(src, (q15_t *)dest, src_len_in_samps);
        return src_len_in_samps * 2;
    }
    else // it must by 24 bit
    {
        data_converters_q31_to_i24(src, dest, src_len_in_samps);
        return src_len_in_samps * 3;
    }
}
