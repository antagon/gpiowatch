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
#include <sys/time.h>
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
 -c <file> load a configuration file\n\
 -v        show version information\n\
 -h        show usage information\n", p);
}

static void
trigger_alarm (int signo) { return; }

int
main (int argc, char *argv[])
{
	char buff[4];
	const char *conf_path;
	struct itimerval timer;
	struct config_entry *config, *config_iter;
	struct config_error config_err;
	struct pollfd fds[32];
	uint32_t pin_state, bit_weight;
	int ret, i, pollres;

	config = NULL;
	ret = EXIT_SUCCESS;
	conf_path = NULL;
	pin_state = 0;

	signal (SIGTERM, interrupt);
	signal (SIGINT, interrupt);
	signal (SIGALRM, trigger_alarm);

	while ( (i = getopt (argc, argv, "c:vh")) != -1 ){
		switch ( i ){
			case 'c':
				conf_path = optarg;
				break;

			case 'v':
				version (argv[0]);
				ret = EXIT_SUCCESS;
				return ret;

			case 'h':
				usage (argv[0]);
				ret = EXIT_SUCCESS;
				return ret;
		}
	}

	/* Init the pollfd structure */
	for ( i = 0; i < sizeof (fds) / sizeof (fds[0]); i++ ){
		fds[i].events = POLLPRI;
		fds[i].revents = 0;
		fds[i].fd = -1;
	}

	openlog (argv[0], LOG_NDELAY | LOG_NOWAIT | LOG_PERROR | LOG_PID, LOG_USER);

	if ( conf_path == NULL ){
		fprintf (stderr, "%s: no configuration file. Try `%s -h` for more information.\n", argv[0], argv[0]);
		ret = EXIT_FAILURE;
		goto egress;
	}

	if ( config_parse (conf_path, &config, &config_err) == -1 ){
		if ( errno )
			fprintf (stderr, "%s: cannot read configuration file '%s': %s\n", argv[0], conf_path, strerror (errno));
		else
			fprintf (stderr, "%s: %s in '%s' on line %d, near character no. %d\n", argv[0], config_err.errmsg, conf_path, config_err.eline, config_err.echr);
		ret = EXIT_FAILURE;
		goto egress;
	}

	if ( sysfs_gpio_init () == -1 ){
		fprintf (stderr, "%s: sysfs interface for GPIO does not exist\n", argv[0]);
		ret = EXIT_FAILURE;
		return ret;
	}

	for ( i = 0; i < sizeof (fds) / sizeof (fds[0]); i++ ){
		bit_weight = (uint32_t) pow (2, i);

		for ( config_iter = config; config_iter != NULL; config_iter = config_iter->next ){
			if ( config_iter->pin_mask & bit_weight ){
				if ( sysfs_gpio_export (i) == -1 ){
					fprintf (stderr, "%s: cannot export GPIO-%d interface: %s\n", argv[0], i, strerror (errno));
					ret = EXIT_FAILURE;
					goto egress;
				}

				if ( sysfs_gpio_set_direction (i, GPIO_DIN) == -1 ){
					fprintf (stderr, "%s: cannot set direction of GPIO-%d interface: %s\n", argv[0], i, strerror (errno));
					ret = EXIT_FAILURE;
					goto egress;
				}

				if ( sysfs_gpio_set_edge (i, GPIO_EDGBOTH) == -1 ){
					fprintf (stderr, "%s: cannot set edge for GPIO-%d interface: %s\n", argv[0], i, strerror (errno));
					ret = EXIT_FAILURE;
					goto egress;
				}

				fds[i].fd = sysfs_gpio_open (i);

				if ( fds[i].fd == -1 ){
					fprintf (stderr, "%s: cannot read from GPIO-%d interface: %s\n", argv[0], i, strerror (errno));
					ret = EXIT_FAILURE;
					goto egress;
				}
				break;
			}
		}
	}

	// Set timer
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 900000;
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 1;

	if ( setitimer (ITIMER_REAL, &timer, NULL) == -1 ){
		fprintf (stderr, "%s: cannot set interval timer: %s\n", argv[0], strerror (errno));
		ret = EXIT_FAILURE;
		goto egress;
	}

	syslog (LOG_INFO, "Started listening events on GPIO pins");

	while ( ! sigexitloop ){
		pollres = poll (fds, sizeof (fds) / sizeof (fds[0]), -1);

		if ( pollres == -1 ){
			if ( errno == EINTR ){
				/* Compare whether state of pins matches a configured pin mask. */
				for ( config_iter = config; pin_state != 0x00 && config_iter != NULL; config_iter = config_iter->next ){
					/* Skip any that do not match the current pin state mask. */
					if ( (config_iter->pin_mask & pin_state) != config_iter->pin_mask ){
						config_iter->state.threshold = 0;
						continue;
					}

					if ( config_iter->state.threshold == config_iter->threshold_sec ){
						fprintf (stderr, "EXEC %s\n", config_iter->cmd);
						// TODO: vvv--- fork and other shenanigans will follow ---vvv
					}

					// FIXME: possible integer overflow!
					config_iter->state.threshold += 1;
				}
				continue;
			}

			syslog (LOG_ERR, "poll error: %s\n", strerror (errno));
			ret = EXIT_FAILURE;
			goto egress;
		}

		if ( pollres > 0 ){
			/* Read input from GPIO pins. */
			for ( i = 0; i < sizeof (fds) / sizeof (fds[0]); i++ ){
				if ( fds[i].revents & POLLPRI ){
					uint8_t boolval;

					if ( read (fds[i].fd, buff, sizeof (buff)) == -1 ){
						syslog (LOG_ERR, "reading from a file failed: %s\n", strerror (errno));
						ret = EXIT_FAILURE;
						goto egress;
					}

					if ( lseek (fds[i].fd, 0, SEEK_SET) == -1 ){
						syslog (LOG_ERR, "lseek error: %s\n", strerror (errno));
						ret = EXIT_FAILURE;
						goto egress;
					}

					boolval = (buff[0] - '0') ? 1:0;
					pin_state ^= (-boolval ^ pin_state) & (1 << (i));
				}
			}
		}
	}

	syslog (LOG_NOTICE, "Stopped listening events on GPIO pins (killed with signal %d)", sigexitloop);

egress:
	for ( i = 0; i < sizeof (fds) / sizeof (fds[0]); i++ ){
		if ( fds[i].fd == -1 )
			continue;

		if ( close (fds[i].fd) == -1 ){
			syslog (LOG_WARNING, "cannot close GPIO-%d interface: %s\n", i, strerror (errno));
			ret = EXIT_FAILURE;
			continue;
		}

		if ( sysfs_gpio_unexport (i) == -1 ){
			syslog (LOG_WARNING, "cannot unexport GPIO-%d interface: %s\n", i, strerror (errno));
			ret = EXIT_FAILURE;
			continue;
		}
	}

	config_free (config);

	closelog ();

	return (sigexitloop) ? sigexitloop:ret;
}

