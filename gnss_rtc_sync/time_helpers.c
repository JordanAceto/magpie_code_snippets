
/* Private includes --------------------------------------------------------------------------------------------------*/

#include "time_helpers.h"

/* Public function definitions ---------------------------------------------------------------------------------------*/

tm_t time_helpers_get_default_time()
{
    const tm_t default_time = {
        .tm_hour = 0,
        .tm_isdst = -1,
        .tm_mday = 1,
        .tm_min = 0,
        .tm_mon = 0,
        .tm_sec = 0,
        .tm_wday = 0,
        .tm_yday = 0,
        .tm_year = 2000 - 1900, // use 2000 as the default year, because that's where the DS3231 starts
    };

    return default_time;
}

Clock_Time_Comparison_t time_helpers_compare_time(const tm_t *lhs, const tm_t *rhs)
{
    // copy so that mktime doesn't mutate the values
    tm_t lhs_copy = *lhs;
    tm_t rhs_copy = *rhs;

    time_t lhs_ = mktime(&lhs_copy);
    time_t rhs_ = mktime(&rhs_copy);

    if (lhs_ < rhs_)
    {
        return CLOCK_TIME_COMPARISON_LHS_IS_EARLIER;
    }
    else if (lhs_ > rhs_)
    {
        return CLOCK_TIME_COMPARISON_LHS_IS_LATER;
    }
    else
    {
        return CLOCK_TIME_COMPARISON_TIMES_ARE_EQUAL;
    }
}

tm_t time_helpers_add_time(const tm_t *t0, int days, int hours, int minutes, int seconds)
{
    // convert to UTC seconds
    tm_t t0_ = *t0;
    time_t t = mktime(&t0_) + (days * 86400) + (hours * 3600) + (minutes * 60) + seconds;

    // and then back to a time struct
    tm_t *t_ = localtime(&t);
    tm_t retval = *t_;
    return retval;
}

size_t time_helpers_tm_to_string(const tm_t *time, char *str_buff)
{
    return strftime(str_buff, 16, "%Y%m%d_%H%M%S", time);
}

int time_helpers_bcd8_byte_to_decimal(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0Fu);
}

uint8_t time_helpers_decimal_0_99_to_bcd8(int decimal)
{
    return ((decimal / 10) << 4) | (decimal % 10);
}
