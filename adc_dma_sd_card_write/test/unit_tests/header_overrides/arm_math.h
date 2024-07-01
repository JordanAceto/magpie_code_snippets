/**
 * This file is a header override for arm_math.h, we only need a couple things from it. Add more here if necessary.
 */

#include <stdint.h>

typedef int32_t q31_t;

#define __SIMD32(addr) (*(int32_t **)&(addr))
