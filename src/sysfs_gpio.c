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

	len = snprintf (buff, sizeof (buff), "%s", (direction) ? "out":"in");

	if ( write (fd, buff, len) == -1 ){
		ret = -1;
		goto egress;
	}

egress:
	close (fd);

	return ret;
}

int
sysfs_gpio_read (int gpio)
{
	char path[64];
	int fd;

	snprintf (path, sizeof (path), "/sys/class/gpio/gpio%d/value", gpio);

	fd = open (path, O_RDONLY | O_NDELAY);

	return fd;
}

