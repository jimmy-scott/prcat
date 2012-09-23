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
#include <termios.h>

#include "xgetpass.h"

/*
 * Get a password from a stream.
 *
 * This function will disable echoing to the terminal and ask for a
 * password on the stream passed to the function. It will only ask the
 * password if echoing was succesfully disabled. Terminal settings are
 * restored after the password was read. Storage will be allocated.
 *
 * Returns password length or -1 on error.
 *
 */

char *
xgetpass(FILE *stream)
{
	char *pw = NULL;
	size_t linecap = 0;
	ssize_t nread;
	struct termios old, new;
	
	/* Get termios structure */
	if (tcgetattr(fileno(stream), &old) != 0)
		return NULL;
	
	/* Make a copy with echo disabled */
	new = old;
	new.c_lflag &= ~ECHO;
	
	/* Set termios structure -- disables echo */
	if (tcsetattr(fileno(stream), TCSAFLUSH, &new) != 0)
		return NULL;
	
	/* Read password from stream */
	nread = getline(&pw, &linecap, stream);
	
	/* Restore original termios structure */
	tcsetattr(fileno(stream), TCSAFLUSH, &old);
	
	/* read error or eof */
	if (nread < 0) {
		if (pw)
			free(pw);
		return NULL;
	}
	
	/* Remove newline from password */
	if (nread > 0 && pw[nread-1] == '\n')
		pw[nread-1] = '\0';
	
	return pw;
}

