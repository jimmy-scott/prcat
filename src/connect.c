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
#include <sys/socket.h>

#include <netinet/in.h>

#include <err.h>
#include <netdb.h>
#include <strings.h>
#include <sysexits.h>
#include <unistd.h>

#include "connect.h"

/*
 * Make a TCP connection to host:port. Returns the file descriptor of
 * the socket if a connection could be established. Never returns if
 * there was an error.
 */

int
tcp_connect(char *host, int port)
{
	int sock;			/* socket file descriptor */
	struct hostent *hent;		/* host lookup information */
	struct sockaddr_in addr;	/* host connect information */
	
	/* get hostent from hostname or address */
	if((hent = gethostbyname(host)) == NULL)
		errx(EX_NOHOST, "%s: hostname lookup failed", host);
	
	/* create inet tcp socket */
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		err(EX_OSERR, "socket() call failed");
	
	/* sockaddr_in: init to zero */
	bzero(&addr, sizeof(addr));
	/* sockaddr_in: set sin_family */
	addr.sin_family = AF_INET;
	/* sockaddr_in: set sin_addr.s_addr */
	bcopy(hent->h_addr, &addr.sin_addr.s_addr, hent->h_length);
	/* sockaddr_in: set sin_port */
	addr.sin_port = htons(port);
	
	/* connect to host */
	if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		close(sock);	/* cleanup */
		err(EX_TEMPFAIL, "failed to connect to %s:%i", host, port);
	}
	
	/* all ok */
	return sock;
}

