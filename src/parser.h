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

#ifndef _PARSER_H_
#define _PARSER_H_

/* holds a key/value pair */
typedef struct parser_item_t {
	char *key;	/* name of the key */
	char *value;	/* name of the value */
} parser_item_t;

/* holds parsed data and list of key/value pairs */
typedef struct parser_t {
	char *data;			/* data read from file */
	int keys;			/* amount of keys found */
	int length;			/* array length of items */
	struct parser_item_t *items;	/* ptr to array of key/values */
} parser_t;

void parser_init(struct parser_t *pd);
void parser_free(struct parser_t *pd);
void parser_destroy(struct parser_t *pd);

int parser_grow(struct parser_t *pd, int size);
int parser_store(struct parser_t *pd, char *key, char *value);
int parser_parse(struct parser_t *pd, char *filename);

char *parser_lookup(struct parser_t *pd, char *key);

#endif /* _PARSER_H_ */
