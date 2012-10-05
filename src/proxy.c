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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "porting.h"

#include "base64.h"
#include "buffer.h"
#include "proxy.h"

/*
 * Returns base64 encoded "username:password" or NULL on error.
 * Both username and password MUST point to a string.
 */

static char *
proxy_basic_auth_token(char *username, char *password)
{
	char *up, *auth;
	size_t ulen, plen;
	
	ulen = strlen(username);
	plen = strlen(password);
	
	/* check for overflow */
	if (SIZE_MAX - ulen < plen || (SIZE_MAX - ulen) - plen < 2)
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

/*
 * Setup a proxy tunnel using HTTP CONNECT.
 *
 * Function sends an HTTP CONNECT header to the socket file descriptor
 * passed to this function. It will attempt to open a tunnel to the
 * hostname and (host)port passed to this function. If username and
 * password are not NULL, they will be used for basic authentication.
 *
 * This function will also check the response. If the proxy responds
 * with "HTTP/1.x 200", the function will return 0. In the other case,
 * or in case of an error, it will return -1.
 */

int
proxy_connect(int sock, struct buffer_t *b, char *hostname,
	int hostport, char *username, char *password)
{
	int slen, nread, hlen, eoflen = 4;
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
		slen = snprintf(b->data, sizeof(b->data),
			"CONNECT %s:%i HTTP/1.0\r\n"
			"Proxy-Authorization: Basic %s\r\n"
			"\r\n", hostname, hostport, auth);
		free(auth);
	} else {
		slen = snprintf(b->data, sizeof(b->data),
			"CONNECT %s:%i HTTP/1.0\r\n"
			"\r\n", hostname, hostport);
	}
	
	if (slen >= sizeof(b->data)) {
		warnx("http send headers too long");
		return -1;
	}

	/* update the length of stored bytes */
	b->s_len = slen;
	
	/* send headers */
	b->w_len = write(sock, b->data, b->s_len);
	
	if (b->w_len == 0) {
		warnx("http send headers failed: eof from proxy");
		return -1;
	} else if (b->w_len == -1) {
		warn("http send headers failed");
		return -1;
	} else if (b->w_len != b->s_len) {
		warn("http send headers failed: short write");
		return -1;
	}
	
	/* receive headers */
	for (b->s_len = 0, bp = b->data; /* forever */ ; bp += nread)
	{
		/* read header(s) */
		nread = read(sock, bp, sizeof(b->data) - b->s_len);
		
		if (nread == 0) {
			warnx("http read headers failed: eof from proxy");
			return -1;
		} else if (nread == -1) {
			warn("http read headers failed"); /* errno knows */
			return -1;
		}
		
		/* read was ok, count the read bytes */
		b->s_len += nread;
		
		/* avoid undefined behaviour in memmem */
		if (b->s_len < 4)
			continue;
		
		/* check if end of headers received */
		if (b->s_len - nread < 3) {
			/* search entire buffer */
			if ((ep = memmem(b->data, b->s_len, "\r\n\r\n", 4)))
				break; /* end of headers found */
#ifdef PROXY_HEADER_END_ALLOW_LFLF
			if ((ep = memmem(b->data, b->s_len, "\n\n", 2))) {
				eoflen = 2;
				break; /* end of headers found */
			}
#endif /* PROXY_HEADER_END_ALLOW_LFLF */
		} else {
			/* search part of the buffer */
			if ((ep = memmem(bp - 3, nread + 3, "\r\n\r\n", 4)))
				break; /* end of headers found */
#ifdef PROXY_HEADER_END_ALLOW_LFLF
			if ((ep = memmem(bp - 1, nread + 1, "\n\n", 2))) {
				eoflen = 2;
				break; /* end of headers found */
			}
#endif /* PROXY_HEADER_END_ALLOW_LFLF */
		}
		
		if (b->s_len == sizeof(b->data)) {
			warnx("http read headers failed: buffer too small");
			return -1;
		}
	}
	
	/* calculate the length of the headers by pointing back to
	 * the beginning of the buffer, substracting that pointer from
	 * the pointer that points to where the end of header mark was
	 * found and adding the length of the end of header mark */
	bp = b->data;
	hlen = (ep - bp) + eoflen;

	/* set the w_len to the length of the headers, so in the case
	 * the return code was 200, it will be clear for the caller
	 * whether there are still bytes in the buffer that need to
	 * be handled */
	b->w_len = hlen;
	
	/* check if response was HTTP/1.x 200 */
	if (strncmp(b->data, "HTTP/1.", 7) == 0 && strncmp(b->data + 9, "200", 3) == 0) {
		/* check if we read data from past the headers */
		if (hlen != b->s_len) {
			warnx("http read headers failed: needs to be fixed: "
				"read %i bytes too much", b->s_len - b->w_len);
			return -1;
		}
		return 0; /* return OK */
	}
	
	/*** it was not 200 ;-( ***/
	
	int pos;
	
	/* print error message by terminating first header with '\0' */
	for (pos = 0; pos < b->s_len; ++pos, ++bp)
	{
#ifdef PROXY_HEADER_END_ALLOW_LFLF
		if (*bp == '\r' || *bp == '\n') {
#else
		if (*bp == '\r') {
#endif
			*bp = '\0';
			warnx("proxy connect failed: %s", b->data);
			return -1;
		}
	}
	
	/* should never be reached */
	warnx("proxy connect failed: unexpected, plz debug");
	return -1;
}

