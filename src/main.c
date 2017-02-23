/*
 * main.c -- gpiowatch main program.
 *
 * Copyright (C) 2017 CodeWard.org
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <poll.h>
#include <syslog.h>
#include <getopt.h>
#include <unistd.h>

#include "sysfs_gpio.h"

int
main (int argc, char *argv[])
{
	char buff[16];
	char gpio_id[] = { 17, 27, 22, 18, 23, 24, 25, 5, 6, 13, 19, 26, 12, 16, 20, 21 };
	struct pollfd fds[sizeof (gpio_id) / sizeof (gpio_id[0])];
	uint32_t pin_state;
	int ret, i, pollres;

	ret = EXIT_SUCCESS;
	pin_state = 0;

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

		if ( sysfs_gpio_set_edge (gpio_id[i], GPIO_EDGBOTH) == -1 ){
			fprintf (stderr, "cannot set edge for GPIO (%d): %s\n", gpio_id[i], strerror (errno));
			ret = EXIT_FAILURE;
			goto egress;
		}

		fds[i].events = POLLPRI;
		fds[i].revents = 0;
		fds[i].fd = sysfs_gpio_open (gpio_id[i]);

		if ( fds[i].fd == -1 ){
			fprintf (stderr, "cannot read from GPIO (%d) interface: %s\n", gpio_id[i], strerror (errno));
			ret = EXIT_FAILURE;
			goto egress;
		}
	}

	for ( ;; ){
		pollres = poll (fds, sizeof (fds) / sizeof (fds[0]), 1000);

		switch ( pollres ){
			case -1:
				fprintf (stderr, "poll failed: %s\n", strerror (errno));
				ret = EXIT_FAILURE;
				goto egress;

			case 0:
				continue;
		}

		for ( i = 0; i < sizeof (fds) / sizeof (fds[0]); i++ ){
			if ( fds[i].revents & POLLPRI ){
				if ( read (fds[i].fd, buff, sizeof (buff)) == -1 ){
					fprintf (stderr, "read failed: %s\n", strerror (errno));
					ret = EXIT_FAILURE;
					goto egress;
				}

				fprintf (stderr, "have data on GPIO #%d (%d)\n", gpio_id[i], buff[0] - '0');

				pin_state ^= (-(buff[0] - '0') ^ pin_state) & (1 << (gpio_id[i]));

				/* Set file pointer back to the beginning. */
				if ( lseek (fds[i].fd, 0, SEEK_SET) == -1 ){
					fprintf (stderr, "lseek failed: %s\n", strerror (errno));
					ret = EXIT_FAILURE;
					goto egress;
				}
			}
		}

		fprintf (stderr, "PINSTATE: %08x\n", pin_state);
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

