/*
 * sysfs_gpio.c -- sysfs interface for manipulation GPIO pins.
 *
 * Copyright (C) 2017 CodeWard.org
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "sysfs_gpio.h"

enum {
	GPIO_UNEXPORT = 0,
	GPIO_EXPORT = 1
};

static int
sysfs_gpio_do (int gpio, int what)
{
	char path[64];
	char buff[8];
	ssize_t len;
	int fd, ret;

	ret = 0;

	snprintf (path, sizeof (path), "/sys/class/gpio/%s", (what == GPIO_EXPORT) ? "export":"unexport");

	fd = open (path, O_WRONLY);

	if ( fd == -1 )
		return -1;

	len = snprintf (buff, sizeof (buff), "%d", gpio);

	if ( write (fd, (char*) buff, len) == -1 ){
		ret = -1;
		goto egress;
	}

egress:
	close (fd);

	return ret;
}

int
sysfs_gpio_init (void)
{
	return access ("/sys/class/gpio/export", F_OK);
}

int
sysfs_gpio_export (int gpio)
{
	return sysfs_gpio_do (gpio, GPIO_EXPORT);
}

int
sysfs_gpio_unexport (int gpio)
{
	return sysfs_gpio_do (gpio, GPIO_UNEXPORT);
}

int
sysfs_gpio_set_direction (int gpio, int direction)
{
	char path[64];
	char buff[8];
	ssize_t len;
	int fd, ret;

	ret = 0;

	snprintf (path, sizeof (path), "/sys/class/gpio/gpio%d/direction", gpio);

	fd = open (path, O_WRONLY);

	if ( fd == -1 )
		return -1;

	len = snprintf (buff, sizeof (buff), "%s", (direction == GPIO_DOUT) ? "out":"in");

	if ( write (fd, buff, len) == -1 ){
		ret = -1;
		goto egress;
	}

egress:
	close (fd);

	return ret;
}

int
sysfs_gpio_set_edge (int gpio, int edge)
{
	char path[64];
	char buff[16];
	const char *edge_strval;
	ssize_t len;
	int fd, ret;

	ret = 0;

	switch ( edge ){
		case GPIO_EDGNONE:
			edge_strval = "none";
			break;

		case GPIO_EDGRISING:
			edge_strval = "rising";
			break;

		case GPIO_EDGFALLING:
			edge_strval = "falling";
			break;

		case GPIO_EDGBOTH:
			edge_strval = "both";
			break;

		default:
			return -1;
	}

	snprintf (path, sizeof (path), "/sys/class/gpio/gpio%d/edge", gpio);

	fd = open (path, O_WRONLY);

	if ( fd == -1 )
		return -1;

	len = snprintf (buff, sizeof (buff), "%s", edge_strval);

	if ( write (fd, buff, len) == -1 ){
		ret = -1;
		goto egress;
	}

egress:
	close (fd);

	return ret;
}

int
sysfs_gpio_open (int gpio)
{
	char path[64];
	int fd;

	snprintf (path, sizeof (path), "/sys/class/gpio/gpio%d/value", gpio);

	fd = open (path, O_RDONLY | O_NONBLOCK);

	return fd;
}

