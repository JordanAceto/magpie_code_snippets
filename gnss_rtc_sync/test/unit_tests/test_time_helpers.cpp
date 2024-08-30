#include <gtest/gtest.h>
#include <gmock/gmock.h>

extern "C"
{
#include "time_helpers.h"
}

// a buffer to store strings into
static char str_buff[100];

TEST(TimeHelpersTest, check_time_to_string)
{
    tm_t t1 = {
        .tm_sec = 0,
        .tm_min = 0,
        .tm_hour = 0,
        .tm_mday = 1,
        .tm_mon = 0,
        .tm_year = 0,
        .tm_yday = 0,
        .tm_isdst = -1,
    };

    const auto len = time_helpers_tm_to_string(&t1, str_buff);
    ASSERT_EQ(len, 15);
    ASSERT_STREQ(str_buff, "19000101_000000");

    t1.tm_sec = 56;
    t1.tm_min = 34;
    t1.tm_hour = 12;
    t1.tm_mday = 27;
    t1.tm_mon = 8; // September, remember the -1 in the date
    t1.tm_year = 2024 - 1900;

    time_helpers_tm_to_string(&t1, str_buff);
    ASSERT_STREQ(str_buff, "20240927_123456");
}

TEST(TimeHelpersTest, compare_time_basic_cases)
{
    tm_t t1 = {
        .tm_sec = 0,
        .tm_min = 0,
        .tm_hour = 0,
        .tm_mday = 1,
        .tm_mon = 0,
        .tm_year = 0,
        .tm_yday = 0,
        .tm_isdst = -1,
    };

    tm_t t2 = {
        .tm_sec = 1,
        .tm_min = 0,
        .tm_hour = 0,
        .tm_mday = 1,
        .tm_mon = 0,
        .tm_year = 0,
        .tm_yday = 0,
        .tm_isdst = -1,
    };

    ASSERT_EQ(time_helpers_compare_time(&t1, &t2), CLOCK_TIME_COMPARISON_LHS_IS_EARLIER);

    t1.tm_sec = 2;
    ASSERT_EQ(time_helpers_compare_time(&t1, &t2), CLOCK_TIME_COMPARISON_LHS_IS_LATER);

    t2.tm_sec = 2;
    ASSERT_EQ(time_helpers_compare_time(&t1, &t2), CLOCK_TIME_COMPARISON_TIMES_ARE_EQUAL);

    t1.tm_min = 1;
    ASSERT_EQ(time_helpers_compare_time(&t1, &t2), CLOCK_TIME_COMPARISON_LHS_IS_LATER);

    t2.tm_hour = 1;
    ASSERT_EQ(time_helpers_compare_time(&t1, &t2), CLOCK_TIME_COMPARISON_LHS_IS_EARLIER);

    t1.tm_yday = 5;
    t2.tm_yday = 5;
    ASSERT_EQ(time_helpers_compare_time(&t1, &t2), CLOCK_TIME_COMPARISON_LHS_IS_EARLIER);

    t1.tm_hour = 23;
    ASSERT_EQ(time_helpers_compare_time(&t1, &t2), CLOCK_TIME_COMPARISON_LHS_IS_LATER);
}

TEST(TimeHelpersTest, add_times_adding_zero_leaves_t0_unchanged)
{
    tm_t t0 = {
        .tm_sec = 0,
        .tm_min = 0,
        .tm_hour = 0,
        .tm_mday = 1,
        .tm_mon = 0,
        .tm_year = 0,
        .tm_yday = 0,
        .tm_isdst = -1,
    };

    time_helpers_add_time(&t0, 0, 0, 0, 0);

    time_helpers_tm_to_string(&t0, str_buff);
    ASSERT_STREQ(str_buff, "19000101_000000");

    t0 = {
        .tm_sec = 42,
        .tm_min = 13,
        .tm_hour = 21,
        .tm_mday = 30,
        .tm_mon = 7,
        .tm_year = 2024 - 1900,
        .tm_yday = 243,
        .tm_isdst = -1,
    };

    time_helpers_add_time(&t0, 0, 0, 0, 0);

    time_helpers_tm_to_string(&t0, str_buff);
    ASSERT_STREQ(str_buff, "20240830_211342");
}

TEST(TimeHelpersTest, add_times_with_no_rollover)
{
    tm_t t0 = {
        .tm_sec = 0,
        .tm_min = 0,
        .tm_hour = 0,
        .tm_mday = 1,
        .tm_mon = 0,
        .tm_year = 0,
        .tm_yday = 0,
        .tm_isdst = -1,
    };

    int days = 1;
    int hours = 1;
    int minutes = 1;
    int seconds = 1;

    tm_t t1 = time_helpers_add_time(&t0, days, hours, minutes, seconds);

    time_helpers_tm_to_string(&t1, str_buff);
    ASSERT_STREQ(str_buff, "19000102_010101");

    tm_t t2 = {
        .tm_sec = 42,
        .tm_min = 13,
        .tm_hour = 3,
        .tm_mday = 30,
        .tm_mon = 7,
        .tm_year = 2024 - 1900,
        .tm_wday = 5,
        .tm_yday = 243,
        .tm_isdst = -1,
    };

    tm_t t3 = time_helpers_add_time(&t2, days, hours, minutes, seconds);

    time_helpers_tm_to_string(&t3, str_buff);
    ASSERT_STREQ(str_buff, "20240831_041443");
}

TEST(TimeHelpersTest, add_seconds_with_rollover)
{
    tm_t t0 = {
        .tm_sec = 0,
        .tm_min = 0,
        .tm_hour = 0,
        .tm_mday = 1,
        .tm_mon = 0,
        .tm_year = 0,
        .tm_yday = 0,
        .tm_isdst = -1,
    };

    int days = 0;
    int hours = 0;
    int minutes = 0;
    int seconds = 60;

    tm_t t1 = time_helpers_add_time(&t0, days, hours, minutes, seconds);
    time_helpers_tm_to_string(&t1, str_buff);
    ASSERT_STREQ(str_buff, "19000101_000100");

    seconds = (20 * 60) + 34;
    t1 = time_helpers_add_time(&t0, days, hours, minutes, seconds);
    time_helpers_tm_to_string(&t1, str_buff);
    ASSERT_STREQ(str_buff, "19000101_002034");

    seconds = (3 * 3600) + (14 * 60) + 33;
    t1 = time_helpers_add_time(&t0, days, hours, minutes, seconds);
    time_helpers_tm_to_string(&t1, str_buff);
    ASSERT_STREQ(str_buff, "19000101_031433");

    tm_t t2 = {
        .tm_sec = 42,
        .tm_min = 13,
        .tm_hour = 3,
        .tm_mday = 30,
        .tm_mon = 7,
        .tm_year = 2024 - 1900,
        .tm_wday = 5,
        .tm_yday = 243,
        .tm_isdst = -1,
    };

    seconds = 60;
    tm_t t3 = time_helpers_add_time(&t2, days, hours, minutes, seconds);
    time_helpers_tm_to_string(&t3, str_buff);
    ASSERT_STREQ(str_buff, "20240830_031442");
}

TEST(TimeHelpersTest, check_bcd8_to_decimal)
{
    ASSERT_EQ(time_helpers_bcd8_byte_to_decimal(0x00), 0);
    ASSERT_EQ(time_helpers_bcd8_byte_to_decimal(0x01), 1);
    ASSERT_EQ(time_helpers_bcd8_byte_to_decimal(0x07), 7);

    ASSERT_EQ(time_helpers_bcd8_byte_to_decimal(0x10), 10);

    ASSERT_EQ(time_helpers_bcd8_byte_to_decimal(0x37), 37);
    ASSERT_EQ(time_helpers_bcd8_byte_to_decimal(0x99), 99);
}

TEST(TimeHelpersTest, check_decimal_to_bcd8)
{
    ASSERT_EQ(time_helpers_decimal_0_99_to_bcd8(0), 0x00);
    ASSERT_EQ(time_helpers_decimal_0_99_to_bcd8(1), 0x01);
    ASSERT_EQ(time_helpers_decimal_0_99_to_bcd8(07), 0x07);

    ASSERT_EQ(time_helpers_decimal_0_99_to_bcd8(10), 0x10);

    ASSERT_EQ(time_helpers_decimal_0_99_to_bcd8(37), 0x37);
    ASSERT_EQ(time_helpers_decimal_0_99_to_bcd8(99), 0x99);
}

TEST(TimeHelpersTest, bcd_to_decimal_vice_versa_are_inverse_funcs_0_to_99)
{
    // check that dec-to-bcd is the inverse of bcd-to-dec
    for (int i = 0; i <= 99; i++)
    {
        ASSERT_EQ(time_helpers_bcd8_byte_to_decimal(time_helpers_decimal_0_99_to_bcd8(i)), i);
    }

    // and also the other way around
    for (int tens = 0; tens <= 9; tens++)
    {
        for (int ones = 0; ones <= 9; ones++)
        {
            const uint8_t bcd = (tens << 4) | ones;
            ASSERT_EQ(time_helpers_decimal_0_99_to_bcd8(time_helpers_bcd8_byte_to_decimal(bcd)), bcd);
        }
    }
}
