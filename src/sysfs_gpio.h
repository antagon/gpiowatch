/*
 * sysfs_gpio.h -- sysfs interface for manipulation GPIO pins.
 *
 * Copyright (C) 2017 CodeWard.org
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#ifndef _SYSFS_GPIO_H
#define _SYSFS_GPIO_H

enum
{
	GPIO_DIN = 0,
	GPIO_DOUT = 1
};

enum
{
	GPIO_EDGNONE = 0,
	GPIO_EDGRISING = 1,
	GPIO_EDGFALLING = 2,
	GPIO_EDGBOTH = 3
};

extern int sysfs_gpio_export (int gpio);

extern int sysfs_gpio_unexport (int gpio);

extern int sysfs_gpio_set_direction (int gpio, int direction);

extern int sysfs_gpio_set_edge (int gpio, int edge);

extern int sysfs_gpio_open (int gpio);

#endif

