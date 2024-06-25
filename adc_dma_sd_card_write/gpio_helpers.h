/*
 * gpio_helpers.h
 *
 *  Created on: Jun 18, 2024
 *      Author: jorda
 */

#ifndef GPIO_HELPERS_H_
#define GPIO_HELPERS_H_

#include "mxc_device.h"
#include <stdbool.h>

void gpio_write_pin(const mxc_gpio_cfg_t *pin, bool state);

void gpio_toggle_pin(const mxc_gpio_cfg_t *pin);

bool gpio_read_pin(const mxc_gpio_cfg_t *pin);

#endif /* GPIO_HELPERS_H_ */
