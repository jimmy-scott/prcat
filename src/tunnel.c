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

#include <sys/select.h>

#include <err.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

#include "buffer.h"
#include "tunnel.h"

/*
 * Read data from a file descriptor.
 *
 * Returns number of bytes read. Returns 0 on EOF.
 * Never returns if there is an error.
 */

static short
tunnel_read(struct buffer_t *b, int rfd)
{
	/* read data */
	b->s_len = read(rfd, b->data, sizeof(b->data));
	
	if (b->s_len == -1)
		err(EX_IOERR, "read error");
	
	return b->s_len;
}

/*
 * Write data to a file descriptor.
 *
 * Returns number of bytes written.
 * Never returns if there is an error.
 */

static short
tunnel_write(struct buffer_t *b, int wfd)
{
	/* write data */
	b->w_len = write(wfd, b->data, b->s_len);
	
	if (b->w_len == -1)
		err(EX_IOERR, "write error");
	else if (b->w_len != b->s_len) /* who turned on O_NONBLOCK? */
		err(EX_IOERR, "short write");
	
	return b->w_len;
}

/*
 * Write pending data to a file descriptor.
 *
 * Returns number of bytes flushed.
 * Never returns if there is an error.
 */

static short
tunnel_flush(struct buffer_t *b, int wfd)
{
	short flushed;
	
	/* write data */
	flushed = write(wfd, b->data + b->w_len, b->s_len - b->w_len);
	
	if (flushed == -1)
		err(EX_IOERR, "write error");
	
	/* count written data */
	b->w_len += flushed;
	
	if (b->w_len != b->s_len) /* who turned on O_NONBLOCK? */
		err(EX_IOERR, "short write");
	
	return flushed;
}

/*
 * Read data from one file descriptor, and write it to the other.
 *
 * Returns number of bytes transmitted. Returns 0 on EOF.
 * Never returns if there is an error.
 */

static short
tunnel_tx(struct buffer_t *b, int rfd, int wfd)
{
	/* read data - return on eof */
	if (tunnel_read(b, rfd) == 0)
		return 0;
	
	/* write data - return bytes transfered */
	return tunnel_write(b, wfd);
}

/* 
 * Tunnel data between two file descriptiors.
 *
 * If the buffer contains pending data, it will be written to wfdx.
 */

void
tunnel_handler(struct buffer_t *b, int rfdx, int wfdx, int rfdy, int wfdy)
{
	fd_set read_fds, all_rfds;
	
	/* init fd sets */
	FD_ZERO(&all_rfds);
	FD_SET(rfdx, &all_rfds);
	FD_SET(rfdy, &all_rfds);
	
	/* flush pending data to wfdx */
	if (b->w_len < b->s_len)
		tunnel_flush(b, wfdx);
	
	for (;;)
	{
		/* re-init read set */
		read_fds = all_rfds;
		
		/* wait for input on read set */
		if (select(FD_SETSIZE, &read_fds, NULL, NULL, NULL) == -1)
			err(EX_SOFTWARE, "select failed");
		
		/* input on rfdx: transmit data to wfdy */
		if (FD_ISSET(rfdx, &read_fds))
			if (tunnel_tx(b, rfdx, wfdy) == 0)
				break;
		
		/* input on rfdy: transmit data to wfdx */
		if (FD_ISSET(rfdy, &read_fds))
			if (tunnel_tx(b, rfdy, wfdx) == 0)
				break;
	}
	
	return;
}

