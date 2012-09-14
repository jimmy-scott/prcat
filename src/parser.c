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

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "readfile.h"
#include "parser.h"

#ifndef PARSER_CUSTOM_CONFIG
#define PARSER_ALLOW_EMPTY_LINES
#define PARSER_ALLOW_COMMENTS
#define PARSER_ALLOW_LEADING_WHITESPACE
#define PARSER_ALLOW_TRAILING_WHITESPACE
#define PARSER_ALLOW_SEPARATING_WHITESPACE
#endif

#ifdef PARSER_ERRORS_ALWAYS_MINUS_ONE
#define PARSE_ERROR(line)	return -1
#else
#define PARSE_ERROR(line)	return line ? line : -1
#endif

/* if you want to allocate up to the max possible items */
// #define PARSER_GROW_MAX_POSSIBLE_ITEMS

/* if you want to allocate up to the max possible bytes */
// #define PARSER_GROW_MAX_POSSIBLE_BYTES
/* note: this will generate warning on architectures where
 *   MAX_INT < (SIZE_T_MAX / sizeof(parser_item_t))
 * it is not harmful, but there is no point enabling this then */

#ifndef PARSER_GROW_FACTOR
#define PARSER_GROW_FACTOR 5
#endif


/*
 * Used by parser_parse. Returns the length of a key.
 */

static int
px_parse_key(char *data)
{
	int len = 0;
	
	/* include allowed characters */
	while (isalnum(*data) || *data == '-') {
		++data;
		++len;
	}
	
	return len;
}

/*
 * Used by parser_parse. Returns the length of a value.
 */

static int
px_parse_value(char *data)
{
	int len = 0;
	
	/* print value */
	while (isalnum(*data)) {
		++data;
		++len;
	}
	
	return len;
}

/*
 * Used by parser_parse. Returns the length of a quoted value.
 */

static int
px_parse_qvalue(char *data)
{
	int len = 0;
	
	/* print value */
	while (*data != '"') {
		/* missing quote or premature end of data */
		if (*data == '\0')
			return -1;
		++data;
		++len;
	}
	
	return len;
}

/*
 * Initialize a parser_t structure for first use.
 */

void
parser_init(struct parser_t *pd)
{
	pd->data = NULL;
	pd->keys = 0;
	pd->length = 0;
	pd->items = NULL;
}

/*
 * Free all memory used by a parser_t structure.
 */

void
parser_free(struct parser_t *pd)
{
	free(pd->data);
	free(pd->items);
}

/*
 * Same as free and init, can be used to avoid double free's.
 */

void
parser_destroy(struct parser_t *pd)
{
	parser_free(pd);
	parser_init(pd);
}

/*
 * Allocate memory for 'size' extra items.
 */

int
parser_grow(struct parser_t *pd, int size)
{
	int alloc_items;
	size_t alloc_bytes;
	struct parser_item_t *items;
	
	/* get alloc_items and check for overflow */
	if (pd->length > (INT_MAX - size)) {
#ifdef PARSER_GROW_MAX_POSSIBLE_ITEMS
		if (pd->length == INT_MAX) {
			errno = EOVERFLOW;
			return -1;
		} else
			alloc_items = INT_MAX;
	} else
		alloc_items = pd->length + size;
#else
		errno = EOVERFLOW;
		return -1;
	}
	
	alloc_items = pd->length + size;
#endif
	
	/* get alloc_bytes and check for overflow */
	if (alloc_items > (SIZE_T_MAX / sizeof(parser_item_t))) {
#ifdef PARSER_GROW_MAX_POSSIBLE_BYTES
		if (pd->length == (SIZE_T_MAX / sizeof(parser_item_t))) {
			errno = EOVERFLOW;
			return -1;
		} else {
			alloc_items = SIZE_T_MAX / sizeof(parser_item_t);
			alloc_bytes = (size_t)alloc_items * sizeof(parser_item_t);
		}
	} else
		alloc_bytes = sizeof(parser_item_t) * (size_t)alloc_items;
#else
		errno = EOVERFLOW;
		return -1;
	}
	
	alloc_bytes = sizeof(parser_item_t) * (size_t)alloc_items;
#endif
	
	/* (re)alloc items */
	if ((items = (parser_item_t*)realloc(pd->items, alloc_bytes)) == NULL)
		return -1;
	
	/* update info */
	pd->items = items;
	pd->length = alloc_items;
	
	//printf("DEBUG: length %i size %zu\n", alloc_items, alloc_bytes);
	
	return 0;
}

/*
 * Store a key/value. Will grow on demand.
 */

int
parser_store(struct parser_t *pd, char *key, char *value)
{
	/* grow items array if needed */
	if (pd->keys == pd->length)
		if (parser_grow(pd, PARSER_GROW_FACTOR) != 0)
			return -1;
	
	/* store values */
	pd->items[pd->keys].key = key;
	pd->items[pd->keys].value = value;
	pd->keys++;
	
	return 0;
}

/*
 * Read and parse file.
 * Will store the key/values and grow on demand.
 */

int
parser_parse(struct parser_t *pd, char *filename)
{
	int line, inquotes = 0;
	char *data, *key, *value;
	unsigned int key_len, value_len;
	
	/* read config file into memory */
	if ((pd->data = readfile(filename)) == NULL)
		return -1;
	
	/* copy to walk over data */
	data = pd->data;
	
	/* parse each line */
	for (line = 1; *data != '\0' ; ++line, ++data)
	{
#ifdef PARSER_ALLOW_LEADING_WHITESPACE
		/* get rid of leading whitespace */
		while (*data == ' ' || *data == '\t')
			++data;
#endif
		
#ifdef PARSER_ALLOW_EMPTY_LINES
		/* empty line -> next */
		if (*data == '\n')
			continue;
#endif
		
#ifdef PARSER_ALLOW_COMMENTS
		/* comment line -> skip -> next */
		if (*data == '#') {
			do {
				++data;
				/* end of data -> stop */
				if (*data == '\0')
					break;
			} while (*data != '\n');
			continue;
		}
#endif
		
		/* key starts here */
		key = data;
		
		/* get key length */
		key_len = px_parse_key(data);
		
		/* if value is invalid */
		if (key_len <= 0)
			PARSE_ERROR(line); /* returns failure */
		
		/* key ok */
		data += key_len;
		
#ifdef PARSER_ALLOW_SEPARATING_WHITESPACE
		/* get rid of whitespace before '=' */
		while (*data == ' ' || *data == '\t')
			++data;
#endif
		
		/* next char must be '=' */
		if (*data == '=')
			++data;
		else
			PARSE_ERROR(line); /* returns failure */
		
#ifdef PARSER_ALLOW_SEPARATING_WHITESPACE
		/* get rid of whitespace after '=' */
		while (*data == ' ' || *data == '\t')
			++data;
#endif
		
		/* if quoted */
		if (*data == '"') {
			++data;
			inquotes = 1;
		}
		
		/* value starts here */
		value = data;
		
		/* get value length */
		value_len = inquotes ?
			px_parse_qvalue(data) : px_parse_value(data);
		
		/* if value is invalid */
		if (value_len <= 0)
			PARSE_ERROR(line); /* returns failure */
		
		/* value ok, skip quote if needed, reset inquotes */
		data += value_len + inquotes;
		inquotes = 0;
		
#ifdef PARSER_ALLOW_TRAILING_WHITESPACE
		/* get rid of trailing whitespace */
		while (*data == ' ' || *data == '\t')
			++data;
#endif
		
		/* must be end of line */
		if (*data != '\n')
			PARSE_ERROR(line); /* returns failure */
		
		/* terminate key/value inline */
		key[key_len] = '\0';
		value[value_len] = '\0';
		
		/* print key/value */
		//printf("%s = \"%s\"\n", key, value);
		
		/* store value */
		if (parser_store(pd, key, value) != 0)
			return -1;
	}
	
	return 0;
}

/*
 * Lookup key. Returns pointer to value or NULL if not found.
 */

char *
parser_lookup(struct parser_t *pd, char *key)
{
	int n;
	
	/* find key and return value */
	for (n = 0; n < pd->keys; ++n) {
		if (strcmp(pd->items[n].key, key) == 0)
			return pd->items[n].value;
	}
	
	/* value not found */
	return NULL;
}

