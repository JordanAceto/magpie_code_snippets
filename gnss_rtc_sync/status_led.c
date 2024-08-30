
/* Private includes --------------------------------------------------------------------------------------------------*/

#include "mxc_device.h"

#include "status_led.h"
#include "gpio_helpers.h"

/* Private variables -------------------------------------------------------------------------------------------------*/

static const mxc_gpio_cfg_t red_led = {
    .port = MXC_GPIO0,
    .mask = MXC_GPIO_PIN_31,
    .pad = MXC_GPIO_PAD_NONE,
    .func = MXC_GPIO_FUNC_OUT,
    .vssel = MXC_GPIO_VSSEL_VDDIOH,
    .drvstr = MXC_GPIO_DRVSTR_0,
};

static const mxc_gpio_cfg_t green_led = {
    .port = MXC_GPIO1,
    .mask = MXC_GPIO_PIN_14,
    .pad = MXC_GPIO_PAD_NONE,
    .func = MXC_GPIO_FUNC_OUT,
    .vssel = MXC_GPIO_VSSEL_VDDIOH,
    .drvstr = MXC_GPIO_DRVSTR_0,
};

static const mxc_gpio_cfg_t blue_led = {
    .port = MXC_GPIO1,
    .mask = MXC_GPIO_PIN_15,
    .pad = MXC_GPIO_PAD_NONE,
    .func = MXC_GPIO_FUNC_OUT,
    .vssel = MXC_GPIO_VSSEL_VDDIOH,
    .drvstr = MXC_GPIO_DRVSTR_0,
};

/* Public function definitions ---------------------------------------------------------------------------------------*/

void status_led_init()
{
    MXC_GPIO_Config(&red_led);
    status_led_set(STATUS_LED_COLOR_RED, false);

    MXC_GPIO_Config(&green_led);
    status_led_set(STATUS_LED_COLOR_GREEN, false);

    MXC_GPIO_Config(&blue_led);
    status_led_set(STATUS_LED_COLOR_BLUE, false);
}

void status_led_set(Status_LED_Color_t color, bool state)
{
    switch (color)
    {
    case STATUS_LED_COLOR_RED:
        gpio_write_pin(&red_led, !state);
        break;
    case STATUS_LED_COLOR_GREEN:
        gpio_write_pin(&green_led, !state);
        break;
    case STATUS_LED_COLOR_BLUE:
        gpio_write_pin(&blue_led, !state);
        break;
    default:
        break;
    }
}

void status_led_toggle(Status_LED_Color_t color)
{
    switch (color)
    {
    case STATUS_LED_COLOR_RED:
        gpio_toggle_pin(&red_led);
        break;
    case STATUS_LED_COLOR_GREEN:
        gpio_toggle_pin(&green_led);
        break;
    case STATUS_LED_COLOR_BLUE:
        gpio_toggle_pin(&blue_led);
        break;
    default:
        break;
    }
}

void status_led_all_off()
{
    status_led_set(STATUS_LED_COLOR_RED, false);
    status_led_set(STATUS_LED_COLOR_GREEN, false);
    status_led_set(STATUS_LED_COLOR_BLUE, false);
}
