#ifndef _SYSFS_GPIO_H
#define _SYSFS_GPIO_H

enum
{
	GPIO_DIN = 0,
	GPIO_DOUT = 1
};

extern int sysfs_gpio_export (int gpio);

extern int sysfs_gpio_unexport (int gpio);

extern int sysfs_gpio_set_direction (int gpio, int direction);

extern int sysfs_gpio_read (int gpio);

#endif

