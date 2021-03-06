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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sysexits.h>

#include "askpass.h"
#include "buffer.h"
#include "setup.h"
#include "connect.h"
#include "tunnel.h"
#include "proxy.h"

#define PASSWORD_PROMPT "Proxy password: "

/*
 * Handles main program flow.
 *
 * Parse command line options and config file data. Ask password if a
 * username was provided, but no password was provided. Connect to the
 * requested HTTP proxy server. Send the required HTTP CONNECT headers
 * to the proxy. Tunnel all data.
 */

int
main(int argc, char **argv)
{
	int sock;
	struct config_t config;
	struct buffer_t buffer;
	
	/* initialize buffer */
	buffer_init(&buffer);
	
	/* parse arguments and config file */
	if (setup(&config, argc, argv) != SETUP_OK) {
		usage(stderr);
		return EX_USAGE;
	}
	
	/* ask password if none was given, but username is set */
	if (config.username && !config.password) {
		config.password = askpass_tty(PASSWORD_PROMPT);
		/* exit if unable to get password */
		if (!config.password)
			return EX_NOINPUT;
	}
	
	/* connect to proxy */
	if ((sock = tcp_connect(config.proxyname, config.proxyport)) == -1)
		return EX_UNAVAILABLE;
	
	/* tunnel setup */
	if (proxy_connect(sock, &buffer, config.hostname, config.hostport,
		config.username, config.password) != 0)
	{
		close(sock);
		return EX_UNAVAILABLE;
	}
	
	/* tunnel data (does not return on failure) */
	tunnel_handler(&buffer, config.ifd, config.ofd, sock, sock);
	
	/* cleanup */
	close(sock);
	
	return EX_OK;
}

