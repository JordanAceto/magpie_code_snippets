/**
 * @file  gpio_helpers.h
 * @brief A software module with a few GPIO convenience functions is represented here.
 */

#ifndef GPIO_HELPERS_H_
#define GPIO_HELPERS_H_

/* Includes ----------------------------------------------------------------------------------------------------------*/

#include "mxc_device.h"
#include <stdbool.h>

/* Public function declarations --------------------------------------------------------------------------------------*/

void gpio_write_pin(const mxc_gpio_cfg_t *pin, bool state);

void gpio_toggle_pin(const mxc_gpio_cfg_t *pin);

bool gpio_read_pin(const mxc_gpio_cfg_t *pin);

#endif /* GPIO_HELPERS_H_ */
