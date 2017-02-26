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
#include <math.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <poll.h>
#include <syslog.h>
#include <getopt.h>
#include <unistd.h>

#include "sysfs_gpio.h"
#include "config.h"

static int sigexitloop = 0;

static void
interrupt (int signo)
{
	sigexitloop = signo;
}

#define GPIOWATCH_VERSION "0.1"

static void
version (const char *p)
{
	fprintf (stdout, "%s %s\n", p, GPIOWATCH_VERSION);
}

static void
usage (const char *p)
{
	fprintf (stdout, "Usage: %s -c <config-file>\n\n\
Options:\n\
 -v  show version information\n\
 -h  show usage information\n", p);
}

int
main (int argc, char *argv[])
{
	char buff[16];
	const char *conf_path;
	struct config_entry *config, *config_iter;
	struct config_error config_err;
	struct pollfd fds[32];
	uint32_t pin_state, bit;
	int ret, i, pollres;

	ret = EXIT_SUCCESS;
	pin_state = 0;
	conf_path = NULL;
	config = NULL;

	/* Init the pollfd structure */
	for ( i = 0; i < sizeof (fds) / sizeof (fds[0]); i++ ){
		fds[i].events = POLLPRI;
		fds[i].revents = 0;
		fds[i].fd = -1;
	}

	signal (SIGTERM, interrupt);
	signal (SIGINT, interrupt);

	while ( (i = getopt (argc, argv, "c:vh")) != -1 ){
		switch ( i ){
			case 'c':
				conf_path = optarg;
				break;

			case 'v':
				version (argv[0]);
				ret = EXIT_SUCCESS;
				goto egress;

			case 'h':
				usage (argv[0]);
				ret = EXIT_SUCCESS;
				goto egress;
		}
	}

	if ( conf_path == NULL ){
		fprintf (stderr, "%s: no configuration file. Try `%s -h` for more information.\n", argv[0], argv[0]);
		ret = EXIT_FAILURE;
		goto egress;
	}

	if ( config_parse (conf_path, &config, &config_err) == -1 ){
		fprintf (stderr, "%s: %s in '%s' on line %d, near character no. %d\n", argv[0], config_err.errmsg, conf_path, config_err.eline, config_err.echr);
		ret = EXIT_FAILURE;
		goto egress;
	}

	for ( i = 0; i < sizeof (fds) / sizeof (fds[0]); i++ ){
		bit = (uint32_t) pow (2, i);

		for ( config_iter = config; config_iter != NULL; config_iter = config_iter->next ){
			if ( config_iter->pin_mask & bit ){
				if ( sysfs_gpio_export (i) == -1 ){
					fprintf (stderr, "cannot export GPIO (%d) interface: %s\n", i, strerror (errno));
					ret = EXIT_FAILURE;
					goto egress;
				}

				if ( sysfs_gpio_set_direction (i, GPIO_DIN) == -1 ){
					fprintf (stderr, "cannot enable GPIO (%d) (INPUT mode): %s\n", i, strerror (errno));
					ret = EXIT_FAILURE;
					goto egress;
				}

				if ( sysfs_gpio_set_edge (i, GPIO_EDGBOTH) == -1 ){
					fprintf (stderr, "cannot set edge for GPIO (%d): %s\n", i, strerror (errno));
					ret = EXIT_FAILURE;
					goto egress;
				}

				fds[i].fd = sysfs_gpio_open (i);

				if ( fds[i].fd == -1 ){
					fprintf (stderr, "cannot read from GPIO (%d) interface: %s\n", i, strerror (errno));
					ret = EXIT_FAILURE;
					goto egress;
				}
				break;
			}
		}
	}

	while ( ! sigexitloop ){
		fprintf (stderr, "waiting for data...\n");
		pollres = poll (fds, sizeof (fds) / sizeof (fds[0]), 1000);

		if ( pollres == -1 ){
			if ( errno == EINTR )
				continue;

			fprintf (stderr, "poll failed: %s\n", strerror (errno));
			ret = EXIT_FAILURE;
			goto egress;
		}

		if ( pollres > 0 ){
			/* Read input from GPIO pins. */
			for ( i = 0; i < sizeof (fds) / sizeof (fds[0]); i++ ){
				if ( fds[i].revents & POLLPRI ){
					if ( read (fds[i].fd, buff, sizeof (buff)) == -1 ){
						fprintf (stderr, "read failed: %s\n", strerror (errno));
						ret = EXIT_FAILURE;
						goto egress;
					}

					fprintf (stderr, "have data on GPIO #%d (%d)\n", i, buff[0] - '0');

					pin_state ^= (-(buff[0] - '0') ^ pin_state) & (1 << (i));

					/* Set file pointer back to the beginning. */
					if ( lseek (fds[i].fd, 0, SEEK_SET) == -1 ){
						fprintf (stderr, "lseek failed: %s\n", strerror (errno));
						ret = EXIT_FAILURE;
						goto egress;
					}
				}
			}
		}

		/* Compare whether state of pins matches a configured pin mask. */
		for ( config_iter = config; pin_state != 0x00 && config_iter != NULL; config_iter = config_iter->next ){
			if ( (config_iter->pin_mask & pin_state) == config_iter->pin_mask ){
				fprintf (stderr, "RUNNING: %s\n", config_iter->cmd);
			}
		}
	}

egress:
	for ( i = 0; i < sizeof (fds) / sizeof (fds[0]); i++ ){
		if ( fds[i].fd == -1 )
			continue;

		if ( close (fds[i].fd) == -1 ){
			fprintf (stderr, "cannot close GPIO (%d) interface: %s\n", i, strerror (errno));
			ret = EXIT_FAILURE;
			continue;
		}

		if ( sysfs_gpio_unexport (i) == -1 ){
			fprintf (stderr, "cannot unexport GPIO (%d) interface: %s\n", i, strerror (errno));
			ret = EXIT_FAILURE;
		}
	}

	config_free (config);

	return ret;
}

