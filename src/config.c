/*
 * config.h -- configuration parser.
 *
 * Copyright (C) 2017 CodeWard.org
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "config.h"

enum {
	PARSE_PIN = 0,
	PARSE_THRSHLD = 1,
	PARSE_CMD = 2
};

static int
iscomment (char sym)
{
	return (sym == '#');
}

static int
isseparator (char sym)
{
	return (sym == '&');
}

static int
isterminator (char sym)
{
	return (sym == ';');
}

static int
parse_line (const char *line, struct config_entry *entry)
{
	size_t i, len, ret;
	int step, digit;

	ret = 0;
	len = strlen (line);
	digit = 0;
	step = PARSE_PIN;

	memset (entry, 0, sizeof (struct config_entry));

	for ( i = 0; i < len; i++, ret++ ){
		// Skip blanks
		if ( isspace (line[i]) && step != PARSE_CMD )
			continue;

		// Skip comment
		if ( iscomment (line[i]) )
			break;

		switch ( step ){
			/* Parse string: 8&22&27, terminated by: ';' */
			case PARSE_PIN:
				if ( isdigit (line[i]) ){
					digit = (digit * 10) + (line[i] - '0');
					continue;
				} else if ( isseparator (line[i]) ){
					// Nothing...
				} else if ( isterminator (line[i]) ){
					step = PARSE_THRSHLD;
				} else {
					entry->echr = ret + 1;
					ret = -1;
					goto egress;
				}

				//entry->pin_mask ^= (-1 ^ entry->pin_mask) & (1 << digit);
				entry->pin_mask = entry->pin_mask & (~(1 << digit) | (1 << digit));
				digit = 0;
				break;

			/* Parse string: 8, terminated by: ';' */
			case PARSE_THRSHLD:
				if ( isdigit (line[i]) ){
					digit = (digit * 10) + (line[i] - '0');
					continue;
				} else if ( isterminator (line[i]) ){
					step = PARSE_CMD;
				} else {
					entry->echr = ret + 1;
					ret = -1;
					goto egress;
				}

				entry->threshold_sec = digit;
				break;

			/* Parse everything left on the line, including the control
			 * characters. */
			case PARSE_CMD:
				if ( entry->cmd_len >= (sizeof (entry->cmd) - 1) ){
					break;
				}

				entry->cmd[entry->cmd_len++] = line[i];
				entry->cmd[entry->cmd_len] = '\0';
				break;
		}
	}

egress:
	return ret;
}

int
config_parse (const char *path, struct config_entry **config)
{
	FILE *file;
	char buff[CONFIG_LINE_MAXLEN];
	struct config_entry parsed;
	int ret;

	file = fopen (path, "r");

	if ( file == NULL )
		return -1;

	while ( fgets (buff, sizeof (buff), file) ){
		// Remove the newline character
		if ( buff[strlen (buff) - 1] == '\n' || buff[strlen (buff) - 1] == '\r' )
			buff[strlen (buff) - 1] = '\0';

		switch ( parse_line (buff, &parsed) ){
			case -1:
				ret = -1;
				goto egress;

			case 0:
				/* Nothing parsed. */
				break;

			default:
				fprintf (stderr, "%s\nMASK: %08x, THRESHOLD: %d, CMD: %s\n", buff, parsed.pin_mask, parsed.threshold_sec, parsed.cmd);
				break;
		}
	}

	if ( ferror (file) ){
		ret = -1;
		goto egress;
	}

egress:
	fclose (file);

	return ret;
}

