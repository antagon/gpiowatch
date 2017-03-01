/*
 * config.h -- configuration parser.
 *
 * Copyright (C) 2017 CodeWard.org
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#ifndef _CONFIG_H
#define _CONFIG_H

#include <stdint.h>
#include <wordexp.h>

#define CONFIG_LINE_MAXLEN 1024
#define CONFIG_CMD_MAXLEN 256

struct config_error
{
	uint32_t echr;
	uint32_t eline;
	const char *errmsg;
};

/* This structure has very little to do with a configuration. However, to have
 * this structure embedded in the configuration structure makes the task of
 * accessing/maintaining current states easiest. */
struct noconfig_state
{
	wordexp_t wargv;
	uint8_t threshold;
};

struct config_entry
{
	uint32_t pin_mask;
	uint8_t threshold_sec;
	uint8_t cmd_len;
	char cmd[CONFIG_CMD_MAXLEN];
	struct noconfig_state state;
	struct config_entry *next;
};

extern int config_parse (const char *path, struct config_entry **config, struct config_error *error);

extern void config_free (struct config_entry *config);

#endif

