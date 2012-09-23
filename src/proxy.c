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

#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "base64.h"
#include "proxy.h"

/*
 * Returns base64 encoded "username:password" or NULL on error.
 * Both username and password need to point to a string.
 */

static char *
proxy_basic_auth_token(char *username, char *password)
{
	char *up, *auth;
	size_t ulen, plen;
	
	ulen = strlen(username);
	plen = strlen(password);
	
	/* check for overflow */
	if (SIZE_T_MAX - ulen < plen || (SIZE_T_MAX - ulen) - plen < 2)
		return NULL;
	
	up = malloc(ulen + plen + 2);
	
	if (!up)
		return NULL;
	
	/* sets 'up' to "username:password" */
	stpcpy(stpcpy(stpcpy(up, username), ":"), password);
	
	/* get base64 encoded string */
	auth = base64(up);
	
	/* cleanup */
	free(up);
	
	/* return encoded string or NULL on base64 error */
	return auth;
}

int
proxy_connect(int sock, char *hostname, int hostport,
	char *username, char *password)
{
	int len, rlen, wlen, hlen, eoflen = 4;
	char buffer[4096];
	char *auth = NULL;
	char *bp, *ep;
	
	/* get auth string if username and password are defined */
	if (username && password) {
		auth = proxy_basic_auth_token(username, password);
		if (!auth) {
			warnx("compose basic auth token failed");
			return -1;
		}
	}
	
	/* compose headers */
	if (auth) {
		len = snprintf(buffer, sizeof(buffer),
			"CONNECT %s:%i HTTP/1.0\r\n"
			"Proxy-Authorization: Basic %s\r\n"
			"\r\n", hostname, hostport, auth);
		free(auth);
	} else {
		len = snprintf(buffer, sizeof(buffer),
			"CONNECT %s:%i HTTP/1.0\r\n"
			"\r\n", hostname, hostport);
	}
	
	if (len >= sizeof(buffer)) {
		warnx("http send headers too long");
		return -1;
	}
	
	/* send headers */
	wlen = write(sock, buffer, len);
	
	if (wlen == 0) {
		warnx("http send headers failed: eof from proxy");
		return -1;
	} else if (wlen == -1) {
		warn("http send headers failed");
		return -1;
	} else if (wlen != len) {
		warn("http send headers failed: short write");
		return -1;
	}
	
	/* receive headers */
	for (len = 0, bp = buffer; /* forever */ ; bp += rlen)
	{
		/* read header(s) */
		rlen = read(sock, bp, sizeof(buffer) - len);
		
		if (rlen == 0) {
			warnx("http read headers failed: eof from proxy");
			return -1;
		} else if (rlen == -1) {
			warn("http read headers failed"); /* errno knows */
			return -1;
		}
		
		len += rlen;
		
		/* avoid undefined behaviour in strnstr */
		if (len < 4)
			continue;
		
		/* check if end of headers received */
		if (len - rlen < 3) {
			/* search entire buffer */
			if ((ep = strnstr(buffer, "\r\n\r\n", len)))
				break; /* end of headers found */
#ifdef PROXY_HEADER_END_ALLOW_LFLF
			if ((ep = strnstr(buffer, "\n\n", len))) {
				eoflen = 2;
				break; /* end of headers found */
			}
#endif /* PROXY_HEADER_END_ALLOW_LFLF */
		} else {
			/* search part of the buffer */
			if ((ep = strnstr(bp - 3, "\r\n\r\n", rlen + 3)))
				break; /* end of headers found */
#ifdef PROXY_HEADER_END_ALLOW_LFLF
			if ((ep = strnstr(bp - 1, "\n\n", rlen + 1))) {
				eoflen = 2;
				break; /* end of headers found */
			}
#endif /* PROXY_HEADER_END_ALLOW_LFLF */
		}
		
		if (len == sizeof(buffer)) {
			warnx("http read headers failed: buffer too small");
			return -1;
		}
	}
	
	bp = buffer;
	hlen = (ep - bp) + eoflen;
	
	/* check if we read data from past the headers */
	if (hlen != len) {
		warnx("http read headers failed: needs to be fixed: "
			"read %i bytes too much", len - hlen);
		return -1;
	}
	
	/* check if response was HTTP/1.x 200 */
	if (strncmp(buffer, "HTTP/1.", 7) == 0 && strncmp(buffer + 9, "200", 3) == 0)
		return 0; /* return OK */
	
	/*** it was not 200 ;-( ***/
	
	int pos;
	
	/* print error message by terminating first header with '\0' */
	for (pos = 0; pos < len; ++pos, ++bp)
	{
#ifdef PROXY_HEADER_END_ALLOW_LFLF
		if (*bp == '\r' || *bp == '\n') {
#else
		if (*bp == '\r') {
#endif
			*bp = '\0';
			warnx("proxy connect failed: %s", buffer);
			return -1;
		}
	}
	
	/* should never be reached */
	warnx("proxy connect failed: unexpected, plz debug");
	return -1;
}

