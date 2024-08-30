
/* Private includes --------------------------------------------------------------------------------------------------*/

#include <stdio.h>

#include "tmr.h"

#include "gnss_module.h"
#include "gpio_helpers.h"
#include "real_time_clock.h"

#include "minmea.h" // 3rd party NMEA parsing lib

/* Private defines ---------------------------------------------------------------------------------------------------*/

#define GNSS_UART_HANDLE (MXC_UART2)
#define GNSS_MODULE_UART_BAUD (9600)

#define START_OF_NMEA_SENTENCE ('$')
#define END_OF_NMEA_SENTENCE ('\n')

/* Private enumerations ----------------------------------------------------------------------------------------------*/

/**
 * @brief enumerated NMEA parser states are represented here. We build up NMEA strings from chars received from the GPS
 * UART.
 */
typedef enum
{
    NMEA_PARSER_STATE_WAITING,         // waiting for the start of sentence char '$
    NMEA_PARSER_STATE_BUILDING_STRING, // in the middle of building up a string
    NMEA_PARSER_STATE_LINE_COMPLETE,   // we saw the end of sentence char '\n'
} NMEA_Parser_State_t;

/* Private variables -------------------------------------------------------------------------------------------------*/

// this pin controls a load switch that powers the GNSS module, high to turn ON, low for OFF
static mxc_gpio_cfg_t gnss_enable_pin = {
    .port = MXC_GPIO0,
    .mask = MXC_GPIO_PIN_23,
    .pad = MXC_GPIO_PAD_NONE,
    .func = MXC_GPIO_FUNC_OUT,
    .vssel = MXC_GPIO_VSSEL_VDDIOH,
    .drvstr = MXC_GPIO_DRVSTR_0,
};

// the PPS signal from the GNSS module pulses high-low once per second when the GPS fix is active
static mxc_gpio_cfg_t gnss_pps_pin = {
    .port = MXC_GPIO0,
    .mask = MXC_GPIO_PIN_24,
    .pad = MXC_GPIO_PAD_NONE,
    .func = MXC_GPIO_FUNC_IN,
    .vssel = MXC_GPIO_VSSEL_VDDIOH,
};

/* Private function declarations -------------------------------------------------------------------------------------*/

/**
 * @brief `is_ascii(c)` is true iff integer `c` represents a valid ascii character
 */
bool is_ascii(int c);

/* Public function definitions ---------------------------------------------------------------------------------------*/

GNSS_Module_Error_t gnss_module_init()
{
    MXC_GPIO_Config(&gnss_enable_pin);

    // turn the module off to start
    gnss_module_disable();

    MXC_GPIO_Config(&gnss_pps_pin);

    if (MXC_UART_Init(GNSS_UART_HANDLE, GNSS_MODULE_UART_BAUD, MAP_B) != E_NO_ERROR)
    {
        return GNSS_MODULE_UART_ERROR;
    }

    // reset the tx and rx pins, they are set to 1.8v domain in MXC_UART_Init(), we want them in the 3V3 domain
    mxc_gpio_cfg_t rx_tx_pins = {
        .port = MXC_GPIO0,
        .mask = MXC_GPIO_PIN_28 | MXC_GPIO_PIN_29,
        .pad = MXC_GPIO_PAD_WEAK_PULL_UP,
        .func = MXC_GPIO_FUNC_ALT3,
        .vssel = MXC_GPIO_VSSEL_VDDIOH,
        .drvstr = MXC_GPIO_DRVSTR_0,
    };
    MXC_GPIO_Config(&rx_tx_pins);

    return GNSS_MODULE_ERROR_ALL_OK;
}

void gnss_module_enable()
{
    gpio_write_pin(&gnss_enable_pin, true);
}

void gnss_module_disable()
{
    gpio_write_pin(&gnss_enable_pin, false);
}

GNSS_Module_Error_t gnss_module_sync_RTC_to_GNSS_time(int timeout_sec)
{
    // a buffer to build the NMEA strings into, chars from the GPS/UART will fill this string
    char nmea_line[MINMEA_MAX_SENTENCE_LENGTH];
    int nmea_str_pos = 0;

    NMEA_Parser_State_t parser_state = NMEA_PARSER_STATE_WAITING;

    // GGA quality is an integer in 0..9
    int gga_quality = 0;

    // TODO: we should probably move the timeout timer into its own module so it can be reused by other modules
    MXC_TMR_TO_Start(MXC_TMR0, timeout_sec * 1000 * 1000); // convert to usec as needed by the MXC function

    while (MXC_TMR_TO_Check(MXC_TMR0) != E_TIME_OUT) // we'll either return from within this loop, or timeout
    {
        // this will either be a char if there is a char from the GNSS and the read is successful, or a negative error
        const int uart_res = MXC_UART_ReadCharacterRaw(GNSS_UART_HANDLE);

        if (!is_ascii(uart_res))
        {
            // skip this round if we get a non-ascii result from the UART read (it returns negative codes on failure)
            continue;
        }

        const char next_char = (char)uart_res;

        switch (parser_state)
        {
        case NMEA_PARSER_STATE_WAITING:

            nmea_str_pos = 0;

            if (next_char == START_OF_NMEA_SENTENCE)
            {
                nmea_line[nmea_str_pos] = next_char;
                nmea_str_pos++;

                parser_state = NMEA_PARSER_STATE_BUILDING_STRING;
            }
            break; // case NMEA_PARSER_STATE_WAITING

        case NMEA_PARSER_STATE_BUILDING_STRING:

            nmea_line[nmea_str_pos] = next_char;
            nmea_str_pos++;

            if (next_char == END_OF_NMEA_SENTENCE)
            {
                nmea_line[nmea_str_pos] = 0; // null-terminate the string
                parser_state = NMEA_PARSER_STATE_LINE_COMPLETE;
            }
            break; // case NMEA_PARSER_STATE_BUILDING_STRING

        case NMEA_PARSER_STATE_LINE_COMPLETE:

            switch (minmea_sentence_id(nmea_line, false))
            {
            case MINMEA_SENTENCE_GGA: // GGA gives us the fix-quality, we can use this to make sure we have a good fix
            {
                struct minmea_sentence_gga frame;
                if (minmea_parse_gga(&frame, nmea_line))
                {
                    gga_quality = frame.fix_quality;
                }

                parser_state = NMEA_PARSER_STATE_WAITING;

                break; // case MINMEA_SENTENCE_GGA
            }
            case MINMEA_SENTENCE_RMC: // RMC has the datetime
            {
                struct minmea_sentence_rmc frame;
                if (minmea_parse_rmc(&frame, nmea_line))
                {
                    // only sync the time if we are sure the GPS signal is good
                    if (gga_quality >= 1 && frame.valid) // TODO: learn more about GNSS, is this a good way to check if we have a good GPS connection?
                    {
                        tm_t gps_time;

                        if (minmea_getdatetime(&gps_time, &frame.date, &frame.time) != E_NO_ERROR)
                        {
                            return GNSS_MODULE_DATETIME_ERROR;
                        }

                        if (real_time_clock_set_datetime(&gps_time) != REAL_TIME_CLOCK_ERROR_ALL_OK)
                        {
                            return GNSS_MODULE_DATETIME_ERROR;
                        }

                        return GNSS_MODULE_ERROR_ALL_OK;
                    }
                }

                break; // case MINMEA_SENTENCE_GGA
            }
            default:
                break;
            } // switch on NMEA sentence

            parser_state = NMEA_PARSER_STATE_WAITING;

            break; // case NMEA_PARSER_STATE_LINE_COMPLETE

        } // switch on parser_State
    } // while !timeout

    return GNSS_MODULE_TIMEOUT_ERROR;
}

/* Private function definitions --------------------------------------------------------------------------------------*/

bool is_ascii(int c)
{
    return (0 <= c && c <= 127);
}
