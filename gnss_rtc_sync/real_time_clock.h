/**
 * @file    real_time_clock.h
 * @brief   A software interface for interacting with the Real Time Clock (RTC) is represented here.
 * @details The RTC is used to tell time, for scheduling recordings and events.
 *
 * This module requires:
 * - Shared use of an I2C bus on the 3V3 domain
 * - Exclusive use of pins P0.13
 */

#ifndef REALTIME_CLOCK_H_
#define REALTIME_CLOCK_H_

/* Includes ----------------------------------------------------------------------------------------------------------*/

#include "i2c.h"

#include "time_helpers.h"

/* Public enumerations -----------------------------------------------------------------------------------------------*/

/**
 * @brief enumerated real time clock errors are represented here.
 */
typedef enum
{
    REAL_TIME_CLOCK_ERROR_ALL_OK,
    REAL_TIME_CLOCK_ERROR_I2C_ERROR,
    REAL_TIME_CLOCK_ERROR_INVALID_ARG_ERROR,
} Real_Time_Clock_Error_t;

/* Public function declarations --------------------------------------------------------------------------------------*/

/**
 * @brief `real_time_clock_init(hi2c)` initializes the real time clock using `hi2c` as the I2C handle for the DS3231.
 * The real time clock is composed of both the external DS3231 chip and the onboard MAX32666 RTC.
 *
 * @pre `hi2c` is configured as an I2C master and has pullup resistors to 3.3V.
 *
 * @param hi2c the I2C handle to use for communication with the DS3231 RTC chip.
 *
 * @post the system RTC is initialized and ready to use, the 32kHz clock from the DS3231 is enabled.
 *
 * @retval `REAL_TIME_CLOCK_ERROR_ALL_OK` if successful, else an enumerated error.
 */
Real_Time_Clock_Error_t real_time_clock_init(mxc_i2c_regs_t *hi2c);

/**
 * @brief `real_time_clock_set_datetime(t)` sets the real time clock to time `t`
 *
 * @pre the real time clock has been initialized.
 *
 * @param new_time the time structure to set as the new time, must be on or after year 2000.
 *
 * @post the real time clock is set to time `t` and immediately starts ticking forward in time.
 *
 * @retval `REAL_TIME_CLOCK_ERROR_ALL_OK` if successful, else an enumerated error.
 */
Real_Time_Clock_Error_t real_time_clock_set_datetime(const tm_t *new_time);

/**
 * @brief `real_time_clock_get_datetime(t)` stores the current real time clock time in time struct `t`.
 *
 * @pre the real time clock has been initialized.
 *
 * @param out_time a pointer to the time structure to fill with the current time, will be mutated.
 *
 * @post the current time is stored in `t`
 *
 * @retval `REAL_TIME_CLOCK_ERROR_ALL_OK` if successful, else an enumerated error.
 */
Real_Time_Clock_Error_t real_time_clock_get_datetime(tm_t *out_time);

/**
 * @brief `real_time_clock_get_milliseconds(ms)` stores the current millisecond value in `ms`, this is always in [0..999].
 *
 * @pre the real time clock has been initialized.
 *
 * @param out_msec a pointer to an integer to store the millisecond value in, this will be mutated.
 *
 * @post the current millisecond value in [0..999] is stored in `out_msec`.
 *
 * @retval `REAL_TIME_CLOCK_ERROR_ALL_OK` if successful, else an enumerated error.
 */
Real_Time_Clock_Error_t real_time_clock_get_milliseconds(int *out_msec);

/**
 * @brief `real_time_clock_set_alarm(t)` sets an alarm to trigger an interrupt at time `t`
 *
 * @pre the real time clock has been initialized, and `t` is in the future.
 *
 * @param alarm_time the time structure to set the alarm.
 *
 * @post an alarm is set to trigger the DS3231 interrupt at time `t`.
 *
 * @retval `REAL_TIME_CLOCK_ERROR_ALL_OK` if successful, else an enumerated error.
 */
Real_Time_Clock_Error_t real_time_clock_set_alarm(const tm_t *alarm_time);

#endif
