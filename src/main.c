#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <syslog.h>
#include <getopt.h>
#include <unistd.h>

#include "sysfs_gpio.h"

int
main (int argc, char *argv[])
{
	char buff[16];
	char gpio_id[] = { 2, 3, 4 };
	struct pollfd fds[sizeof (gpio_id) / sizeof (gpio_id[0])];
	int ret, i, pollres;

	ret = EXIT_SUCCESS;

	for ( i = 0; i < sizeof (gpio_id) / sizeof (gpio_id[0]); i++ ){
		if ( sysfs_gpio_export (gpio_id[i]) == -1 ){
			fprintf (stderr, "cannot export GPIO (%d) interface: %s\n", gpio_id[i], strerror (errno));
			ret = EXIT_FAILURE;
			goto egress;
		}

		if ( sysfs_gpio_set_direction (gpio_id[i], GPIO_DIN) == -1 ){
			fprintf (stderr, "cannot enable GPIO (%d) (INPUT mode): %s\n", gpio_id[i], strerror (errno));
			ret = EXIT_FAILURE;
			goto egress;
		}

		fds[i].fd = sysfs_gpio_read (gpio_id[i]);
		fds[i].events = POLLPRI;
		fds[i].revents = 0;

		if ( fds[i].fd == -1 ){
			fprintf (stderr, "cannot read from GPIO (%d) interface: %s\n", gpio_id[i], strerror (errno));
			ret = EXIT_FAILURE;
			goto egress;
		}
	}

	for ( ;; ){
		pollres = poll (fds, sizeof (fds) / sizeof (fds[0]), -1);

		switch ( pollres ){
			case -1:
				fprintf (stderr, "poll failed: %s\n", strerror (errno));
				ret = EXIT_FAILURE;
				goto egress;
			case 0:
				fprintf (stderr, "poll timeout\n");
				continue;
		}

		for ( i = 0; i < sizeof (fds) / sizeof (fds[0]); i++ ){
			if ( fds[i].revents & POLLPRI ){
				if ( read (fds[i].fd, buff, sizeof (buff)) == -1 ){
					fprintf (stderr, "read failed: %s\n", strerror (errno));
					ret = EXIT_FAILURE;
					goto egress;
				}

				fprintf (stderr, "data -> %s...\n", buff);
			}
		}
	}

egress:
	for ( i = 0; i < sizeof (gpio_id) / sizeof (gpio_id[0]); i++ ){
		if ( sysfs_gpio_unexport (gpio_id[i]) == -1 ){
			ret = EXIT_FAILURE;
		}

		if ( fds[i].fd != -1 )
			close (fds[i].fd);
	}

	return ret;
}

