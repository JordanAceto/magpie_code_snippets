/* Private includes --------------------------------------------------------------------------------------------------*/

#include "gpio_helpers.h"

/* Public function definitions ---------------------------------------------------------------------------------------*/

void gpio_write_pin(const mxc_gpio_cfg_t *pin, bool state)
{
    state ? MXC_GPIO_OutSet(pin->port, pin->mask) : MXC_GPIO_OutClr(pin->port, pin->mask);
}

void gpio_toggle_pin(const mxc_gpio_cfg_t *pin)
{
    MXC_GPIO_OutToggle(pin->port, pin->mask);
}

bool gpio_read_pin(const mxc_gpio_cfg_t *pin)
{
    return (bool)MXC_GPIO_InGet(pin->port, pin->mask);
}
