
/* Private includes --------------------------------------------------------------------------------------------------*/

#include "arm_fir_decimate_fast_q31_bob.h"
#include "audio_dma.h"
#include "data_converters.h"
#include "decimation_filter.h"
#include "gpio_helpers.h" // used for profiling/timing tests

/* Private defines ---------------------------------------------------------------------------------------------------*/

#define buffLen_deci2x (AUDIO_DMA_BUFF_LEN_IN_SAMPS / 2) // after 2x decimation
#define buffLen_deci4x (AUDIO_DMA_BUFF_LEN_IN_SAMPS / 4) // after 4x decimation
#define buffLen_deci8x (AUDIO_DMA_BUFF_LEN_IN_SAMPS / 8) // after 8x decimation

// 1st 2:1 decimator defines
#define decimate_2x_fir_numcoeffs0 (7) // # of filter taps, multiples of 4 run faster
#define decimate_fir_state_len0 (AUDIO_DMA_BUFF_LEN_IN_SAMPS + decimate_2x_fir_numcoeffs0 - 1)

#define decimate_2x_fir_numcoeffs1 (9) // # of filter taps, multiples of 4 run faster
#define decimate_fir_state_len1 (buffLen_deci2x + decimate_2x_fir_numcoeffs1 - 1)

#define decimate_2x_fir_numcoeffs2 (33) // # of filter taps, multiples of 4 run faster
#define decimate_fir_state_len2 (buffLen_deci4x + decimate_2x_fir_numcoeffs2 - 1)

/* Private variables -------------------------------------------------------------------------------------------------*/

// note, 1st stage coefficients have -3dB loss to avoid wrap-around in case of filter overshoot on a transient edge
static q31_t firCoeffs_stage0[decimate_2x_fir_numcoeffs0] = {
    -42201666, 18023525, 423595866, 727113801, 423595866, 18023525, -42201666};

static q31_t firCoeffs_stage1[decimate_2x_fir_numcoeffs1] = {
    -35829136, -93392547, 90204797, 624894336, 955274946, 624894336, 90204797, -93392547, -35829136};

static q31_t firCoeffs_stage2[decimate_2x_fir_numcoeffs2] = {
    -2823963, 804105, 13756249, 13832557, -12099816, -21810016, 17681236, 39284877, -22118934, -66381589, 26258540, 112809109, -29540929, -212849373, 31655722, 678451831, 1041361918, 678451831, 31655722, -212849373, -29540929, 112809109, 26258540, -66381589, -22118934, 39284877, 17681236, -21810016, -12099816, 13832557, 13756249, 804105, -2823963};

// the decimation filter state registers, required by the CMSIS routines
static q31_t firState_stage0[decimate_fir_state_len0] = {0};
static q31_t firState_stage1[decimate_fir_state_len1] = {0};
static q31_t firState_stage2[decimate_fir_state_len2] = {0};

// CMSIS instances. Note that the "fast" version uses the same structure as the regular version
arm_fir_decimate_instance_q31 Sdeci_2x_0;
arm_fir_decimate_instance_q31 Sdeci_2x_1;
arm_fir_decimate_instance_q31 Sdeci_2x_2;

/* Public function definitions ---------------------------------------------------------------------------------------*/

void decimation_filter_init()
{
    arm_fir_decimate_init_q31(&Sdeci_2x_0, decimate_2x_fir_numcoeffs0, 2, &firCoeffs_stage0[0], &firState_stage0[0], AUDIO_DMA_BUFF_LEN_IN_SAMPS);
    arm_fir_decimate_init_q31(&Sdeci_2x_1, decimate_2x_fir_numcoeffs1, 2, &firCoeffs_stage1[0], &firState_stage1[0], buffLen_deci2x);
    arm_fir_decimate_init_q31(&Sdeci_2x_2, decimate_2x_fir_numcoeffs2, 2, &firCoeffs_stage2[0], &firState_stage2[0], buffLen_deci4x);
}

uint32_t decimation_filter_downsample(
    uint8_t *src_384kHz_24_bit,
    uint32_t src_len_in_bytes,
    uint8_t *dest,
    Wave_Header_Sample_Rate_t dest_sample_rate,
    Wave_Header_Bits_Per_Sample_t dest_bit_depth)
{
    // a buffer to assemble the 24 bit samples into 32 bit words
    static q31_t i24_to_q31_buff[AUDIO_DMA_BUFF_LEN_IN_SAMPS] = {0};

    // gpio_profiling_pin_write(true);
    data_converters_i24_to_q31(src_384kHz_24_bit, i24_to_q31_buff, src_len_in_bytes);
    // gpio_profiling_pin_write(false);

    static q31_t dma_rx_deci_2x[buffLen_deci2x] = {0}; // 1st 2X decimator output
    static q31_t dma_rx_deci_4x[buffLen_deci4x] = {0}; // 2nd 2X decimator
    static q31_t dma_rx_deci_8x[buffLen_deci8x] = {0}; // 3rd 2X decimator

    switch (dest_sample_rate)
    {
    case WAVE_HEADER_SAMPLE_RATE_192kHz:
        // first round, 192kHz
        arm_fir_decimate_fast_q31_bob(&Sdeci_2x_0, i24_to_q31_buff, dma_rx_deci_2x, AUDIO_DMA_BUFF_LEN_IN_SAMPS);

        if (dest_bit_depth == WAVE_HEADER_16_BITS_PER_SAMPLE)
        {
            arm_q31_to_q15(dma_rx_deci_2x, (q15_t *)dest, buffLen_deci2x);
            return buffLen_deci2x * 2;
        }
        else // it must by 24 bit
        {
            data_converters_q31_to_i24(dma_rx_deci_2x, dest, buffLen_deci2x);
            return buffLen_deci2x * 3;
        }

    case WAVE_HEADER_SAMPLE_RATE_96kHz:
        // first round, 192kHz
        arm_fir_decimate_fast_q31_bob(&Sdeci_2x_0, i24_to_q31_buff, dma_rx_deci_2x, AUDIO_DMA_BUFF_LEN_IN_SAMPS);
        // second round, 96kHz
        arm_fir_decimate_fast_q31_bob(&Sdeci_2x_1, dma_rx_deci_2x, dma_rx_deci_4x, buffLen_deci2x);

        if (dest_bit_depth == WAVE_HEADER_16_BITS_PER_SAMPLE)
        {
            arm_q31_to_q15(dma_rx_deci_4x, (q15_t *)dest, buffLen_deci4x);
            return buffLen_deci4x * 2;
        }
        else // it must by 24 bit
        {
            data_converters_q31_to_i24(dma_rx_deci_4x, dest, buffLen_deci4x);
            return buffLen_deci4x * 3;
        }

    case WAVE_HEADER_SAMPLE_RATE_48kHz:
        // first round, 192kHz
        arm_fir_decimate_fast_q31_bob(&Sdeci_2x_0, i24_to_q31_buff, dma_rx_deci_2x, AUDIO_DMA_BUFF_LEN_IN_SAMPS);
        // second round, 96kHz
        arm_fir_decimate_fast_q31_bob(&Sdeci_2x_1, dma_rx_deci_2x, dma_rx_deci_4x, buffLen_deci2x);
        // third round, 48kHz
        arm_fir_decimate_fast_q31_bob(&Sdeci_2x_2, dma_rx_deci_4x, dma_rx_deci_8x, buffLen_deci4x);

        if (dest_bit_depth == WAVE_HEADER_16_BITS_PER_SAMPLE)
        {
            arm_q31_to_q15(dma_rx_deci_8x, (q15_t *)dest, buffLen_deci8x);
            return buffLen_deci8x * 2;
        }
        else // it must by 24 bit
        {
            data_converters_q31_to_i24(dma_rx_deci_8x, dest, buffLen_deci8x);
            return buffLen_deci8x * 3;
        }

    default:
        // TODO: handle the other enumerated cases, this may involve generating more FIR coefficients
        return 0;
    }
}
