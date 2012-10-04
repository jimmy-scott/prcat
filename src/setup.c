/*
 * Copyright (C) 2012 Jimmy Scott #jimmy#inet-solutions#be#. Belgium.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  3. The names of the authors may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include "setup.h"
#include "parser.h"
#include "version.h"

/* Macros for validating input. */

#define STRTOL_INVALID_FD(i, str, ep) \
	(*ep || ep == str || i < 0 || i > FD_SETSIZE - 1)
#define STRTOL_INVALID_PORT(i, str, ep) \
	(*ep || ep == str || i < 1 || i > UINT16_MAX)

/* Constants. */

#define UNDEFINED_FD -1
#define CONFIG_FILE ".prcat"

/* Static functions - custom ordering ftw. */

static void config_init(struct config_t *config);
static void config_finalize(struct config_t *config);
static int config_validate(struct config_t *config);
static int parse_args(struct config_t *config, int argc, char **argv);
static int parse_conf(struct config_t *config, char *filename);

/*
 * Print "short" usage information to stream.
 */

void
usage(FILE *stream)
{
	fputs(
	"usage: prcat [opts] <hostname> <port>\n\n"
	"Arguments:\n"
	"  hostname          Connect to this hostname\n"
	"  port              Connect to this port number\n\n"
	"Options:\n"
	"  -u <username>     Username for proxy authentication\n"
	"  -p <password>     Password for proxy authentication\n"
	"  -H <proxy-host>   Connect to this proxy server\n"
	"  -P <proxy-port>   Connect to this port on proxy server\n"
	"  -I <input-fd>     Use this file descriptor for input\n"
	"  -O <output-fd>    Use this file descriptor for ouput\n"
	"  -f <filename>     Use this alternate configuration file\n"
	"  -h                Show this help\n"
	"  -v                Show version\n"
	, stream);
}

/*
 * Parse arguments and config file.
 *
 * An alternate config file can be provided on the command line, in
 * the case it will check if the default config file in the user's
 * home folder exists and parse that one.
 *
 * Values are stored in the config_t structured which is passed as a
 * pointer to the function. Function will return SETUP_OK if all was
 * OK or SETUP_ERROR in case of an error.
 */

int
setup(struct config_t *config, int argc, char **argv)
{
	char *home, *cfgfile = NULL;
	
	config_init(config);
	
	/* parse arguments */
	if (parse_args(config, argc, argv) != 0)
		return SETUP_ERROR;
	
	/* if the user did not supply an alternate config file,
	 * check if the default config file exists */
	if (!config->filename) {
		if ((home = getenv("HOME"))) {
			cfgfile = malloc(strlen(home) + sizeof(CONFIG_FILE) + 2);
			if (!cfgfile) {
				warn("malloc failed for $HOME + " CONFIG_FILE);
				return SETUP_ERROR;
			}
			stpcpy(stpcpy(stpcpy(cfgfile, home), "/"), CONFIG_FILE);
			if (access(cfgfile, F_OK) == 0) {
				/* file exists - keep malloc'ed data */
				config->filename = cfgfile;
			} else {
				/* file does not exist - free data */
				errno = 0;
				free(cfgfile);
				cfgfile = NULL;
			}
		}
	}
	
	/* parse config file - if provided or default exists */
	if (config->filename && parse_conf(config, config->filename) != 0)
		return SETUP_ERROR;
	
	/* put defaults for each undefined variable */
	config_finalize(config);
	
	/* check mandatory/conflicting settings */
	if (config_validate(config) != 0)
		return SETUP_ERROR;
	
	return SETUP_OK;
}

/*
 * Initialize config_t structure to default values.
 */

static void
config_init(struct config_t *config)
{
	/* set everything to 0/NULL */
	memset(config, 0, sizeof(config_t));
	
	/* set to 'undefined' where 0 is a valid value */
	config->ifd = UNDEFINED_FD;
	config->ofd = UNDEFINED_FD;
}

/*
 * Finalize configuration by setting values that are left undefined.
 *
 * This is needed because values defined in the config file need to be
 * able to distinguish whether it was set by an option argument or not.
 */

static void
config_finalize(struct config_t *config)
{
	/* set undefined values to defaults */
	if (config->ifd == UNDEFINED_FD)
		config->ifd = STDIN_FILENO;
	if (config->ofd == UNDEFINED_FD)
		config->ofd = STDOUT_FILENO;
}

/*
 * Verify if mandatory values are set.
 *
 * This will verify all values that may be set by both an option
 * argument and by configuring it in the config file. Logic of
 * conflicting options should also go here. This is not the place
 * to put checks for data that MUST be on the command line.
 *
 * Returns 0 if OK, -1 on error.
 */

static int
config_validate(struct config_t *config)
{
	/* check if mandatory options are set */
	if (!config->proxyname) {
		warnx("missing parameter: proxy hostname");
		return -1;
	}
	if (!config->proxyport) {
		warnx("missing parameter: proxy port");
		return -1;
	}
	
	return 0; /* ok */
}

/*
 * Parse command-line arguments, and set the values in *config.
 *
 * Parses both options and non-option arguments. Will validate input
 * and amount of arguments.
 *
 * Returns 0 if OK, -1 on error. Exits on help/version.
 */

static int
parse_args(struct config_t *config, int argc, char **argv)
{
	int c;
	long num;
	char *endptr = NULL;
	
	/* options */
	static char *shortopts = "hvf:u:p:P:H:I:O:";
	static struct option longopts[] = {
		{ "filename",   required_argument, NULL, 'f' },
		{ "username",   required_argument, NULL, 'u' },
		{ "password",   required_argument, NULL, 'p' },
		{ "proxy-host", required_argument, NULL, 'H' },
		{ "proxy-port", required_argument, NULL, 'P' },
		{ "input-fd",   required_argument, NULL, 'I' },
		{ "output-fd",  required_argument, NULL, 'O' },
		{ "help",       no_argument,       NULL, 'h' },
		{ "version",    no_argument,       NULL, 'v' },
		{ NULL, 0, NULL, 0 }
	};
	
	/* get options from arguments */
	while ((c = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
		switch (c) {
		case 'f':
			config->filename = optarg;
			break;
		case 'u':
			config->username = optarg;
			break;
		case 'p':
			config->password = optarg;
			break;
		case 'H':
			config->proxyname = optarg;
			break;
		case 'P':
			num = strtol(optarg, &endptr, 10);
			if (STRTOL_INVALID_PORT(num, optarg, endptr)) {
				warn("invalid proxy port: %s", optarg);
				return -1;
			}
			config->proxyport = (int)num;
			break;
		case 'I':
			num = strtol(optarg, &endptr, 10);
			if (STRTOL_INVALID_FD(num, optarg, endptr)) {
				warnx("invalid input fd: %s", optarg);
				return -1;
			}
			config->ifd = (int)num;
			break;
		case 'O':
			num = strtol(optarg, &endptr, 10);
			if (STRTOL_INVALID_FD(num, optarg, endptr)) {
				warnx("invalid output fd: %s", optarg);
				return -1;
			}
			config->ofd = (int)num;
			break;
		case 'h':
			usage(stdout);
			exit(EX_OK);
			break;
		case 'v':
			/* too lame to make a function for this */
			puts("prcat " BUILD_VERSION);
			exit(EX_OK);
			break;
		default:
			return -1;
			break;
		}
	}
	
	/* need 2 non-option argument */
	if (argc - optind != 2) {
		warnx("need 2 arguments but got %i", (argc - optind));
		return -1;
	}
	
	/* assign non-option argument #1 = hostname */
	config->hostname = *(argv += optind);
	
	/* assign non-option argument #2 = hostport */
	num = strtol(*(++argv), &endptr, 10);
	if (STRTOL_INVALID_PORT(num, *argv, endptr)) {
		warnx("invalid target port: %s", *argv);
		return -1;
	}
	config->hostport = num;
	
	/* all ok */
	return 0;
}

/*
 * Parse configuration file, and set the values in *config.
 *
 * This function should never be called more than once: it uses static
 * storage for the parser_t structure. The parser_t structure keeps
 * track of the pointers to the file data and key/value pairs.
 *
 * Returns 0 if OK, -1 on error.
 */

static int
parse_conf(struct config_t *config, char *filename)
{
	int status, i;
	long num;
	char *key, *value, *endptr = NULL;
	
	/* parser storage */
	static struct parser_t parser;
	
	/* initialize parser */
	parser_init(&parser);
	
	/* pre-allocate 5 key/value pairs */
	parser_grow(&parser, 5);
	
	/* parse config file */
	status = parser_parse(&parser, filename);
	if (status != 0) {
		if (errno)
			warn("%s: parse failed", filename);
		else
			warnx("%s: parse error on line %i", filename, status);
		return -1;
	}
	
	/* for each option in config file */
	for (i = 0; i < parser.keys; ++i)
	{
		/* easier on the eyes */
		key = parser.items[i].key;
		value = parser.items[i].value;
		
		/* process options */
		if (strcmp(key, "username") == 0)
		{
			/* set if not set */
			if (!config->username)
				config->username = value;
		}
		else if (strcmp(key, "password") == 0)
		{
			/* set if not set */
			if (!config->password)
				config->password = value;
		}
		else if (strcmp(key, "proxy-host") == 0)
		{
			/* set if not set */
			if (!config->proxyname)
				config->proxyname = value;
		}
		else if (strcmp(key, "proxy-port") == 0)
		{
			/* skip if set */
			if (config->proxyport)
				continue;
			
			num = strtol(value, &endptr, 10);
			if (STRTOL_INVALID_PORT(num, value, endptr)) {
				warn("invalid proxy port: %s", value);
				return -1;
			}
			config->proxyport = (int)num;
		}
		else if (strcmp(key, "input-fd") == 0)
		{
			/* skip if set */
			if (config->ifd != UNDEFINED_FD)
				continue;
			
			num = strtol(value, &endptr, 10);
			if (STRTOL_INVALID_FD(num, value, endptr)) {
				warnx("invalid input fd: %s", value);
				return -1;
			}
			config->ifd = (int)num;
		}
		else if (strcmp(key, "output-fd") == 0)
		{
			/* skip if set */
			if (config->ofd != UNDEFINED_FD)
				continue;
			
			num = strtol(value, &endptr, 10);
			if (STRTOL_INVALID_FD(num, value, endptr)) {
				warnx("invalid output fd: %s", value);
				return -1;
			}
			config->ofd = (int)num;
		}
		else
		{
			warnx("%s: invalid keyword: %s", filename, key);
			return -1;
		}
	}
	
	/* all ok */
	return 0;
}
