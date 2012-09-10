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

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "readfile.h"

/* must be defined for check in readfile() */
#ifndef OFF_MAX
	#error OFF_MAX is not defined
#endif
#ifndef SIZE_T_MAX
	#error SIZE_T_MAX is not defined
#endif

/*
 * Returns content of the file, or NULL in case of error.
 * You need to use free() on the pointer to reclaim its memory.
 */

char *
readfile(char *filename)
{
	FILE *fp;
	char *data;
	off_t size;
	
	/* open file for reading */
	if ((fp = fopen(filename, "r")) == NULL)
		return NULL;
	
	/* seek to end, get position, seek to begin */
	if (
		fseek(fp, 0L, SEEK_END) != 0 ||
		(size = ftello(fp)) == -1 ||
		fseek(fp, 0L, SEEK_SET) != 0
	) {
		fclose(fp);
		return NULL;
	}
	
#if OFF_MAX > SIZE_T_MAX
	/* if file is larger than what can be allocated */
	/* sign mismatch, ok since both should be positive */
	if (size >= SIZE_T_MAX) {
#else
	/* check if +1 will overflow */
	if (size == SIZE_T_MAX) {
#endif
		errno = EOVERFLOW;
		fclose(fp);
		return NULL;
	}
	
	/* allocate memory */
	if ((data = (char*)malloc(size + 1)) == NULL) {
		fclose(fp);
		return NULL;
	}
	
	/* read data */
	if (fread(data, sizeof(char), size, fp) != size) {
		free(data);
		fclose(fp);
		return NULL;
	}
	
	/* terminate with null byte */
	data[size] = '\0';
	fclose(fp);
	
	return data;
}

