/*
 * Copyright (C) 2002-2006 Josh A. Beam
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/* base64 characters */
static char b64chars[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
	'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
};

/* return base64 encoded string */
char *
base64(char *s)
{
	int i, j, blen;
	size_t slen;
	unsigned int bits = 0;
	unsigned char tmp[4];
	char *out;

	/* length of s(tring) */
	slen = strlen(s);
	
	/* function does not handle a buffer > INT_MAX;
	 * check max length we can allocate a buffer for */
	if (slen > ((INT_MAX / 4) * 3)) {
		warnx("base64: string too long");
		return NULL;
	}
	
	/* length of b(uffer) - we can now safely do
	 * this calculation without overflow */
	blen = (((slen + 2) / 3) * 4) + 1;

	/* allocate memory for encoded string */
	if ((out = malloc(blen)) == NULL) {
		warn("base64: malloc failed");
		return NULL;
	}

	j = 0;
	for(i = 0; i < slen /* && (j-4) < (blen - 1) */ ; i++) {
		bits <<= 8;
		bits |= s[i] & 0xff;

		if(!((i+1) % 3)) {
			tmp[0] = (bits >> 18) & 0x3f;
			tmp[1] = (bits >> 12) & 0x3f;
			tmp[2] = (bits >> 6) & 0x3f;
			tmp[3] = bits & 0x3f;
			bits = 0;

			out[j++] = b64chars[(tmp[0])];
			out[j++] = b64chars[(tmp[1])];
			out[j++] = b64chars[(tmp[2])];
			out[j++] = b64chars[(tmp[3])];
		}
	}
	switch(i % 3) {
		default:
			break;
		case 2:
			tmp[0] = (bits >> 10) & 0x3f;
			tmp[1] = (bits >> 4) & 0x3f;
			tmp[2] = (bits << 2) & 0x3f;

			out[j++] = b64chars[(tmp[0])];
			out[j++] = b64chars[(tmp[1])];
			out[j++] = b64chars[(tmp[2])];
			out[j++] = '=';
			break;
		case 1:
			tmp[0] = (bits >> 2) & 0x3f;
			tmp[1] = (bits << 4) & 0x3f;

			out[j++] = b64chars[(tmp[0])];
			out[j++] = b64chars[(tmp[1])];
			out[j++] = '=';
			out[j++] = '=';
			break;
	}

	out[j] = '\0';
	return out;
}

