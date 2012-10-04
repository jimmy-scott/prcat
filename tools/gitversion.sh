#!/bin/sh

######
# gitversion.sh: print a usable version based on git data
###
#
# Copyright (C) 2012 Jimmy Scott #jimmy#inet-solutions#be#. Belgium.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  3. The names of the authors may not be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
#
######

# If this doesn't work, we are not inside a git repository, or git is
# not installed or not working correctly.
git status -s > /dev/null 2>&1
[ $? -ne 0 ] && exit 1

# Only release tags should start with 'v', and release tags should be
# annotated and signed by me, so only these should be matched. If the
# working directory is dirty, include it in the tag.
GIT_VERSION=`git describe --dirty --exact-match --match 'v*' 2> /dev/null`

# If an exact match is found, remove the leading 'v', print and quit.
if [ -n "${GIT_VERSION}" ]; then
	echo "${GIT_VERSION}" | cut -c 2-
	exit 0
fi

# If no exact match is found, try to describe the version based on a
# previous annotated tag that starts with 'v'.
GIT_VERSION=`git describe --dirty --abbrev=8 --match 'v*' 2> /dev/null`

# If an exact match is found, flag the version as unstable, remove the
# leading 'v', then print and quit.
if [ -n "${GIT_VERSION}" ]; then
	echo "${GIT_VERSION}-unstable" | cut -c 2-
	exit 0
fi

# If everything else fails, describe it by the sha id.
GIT_VERSION=`git describe --always --dirty --abbrev=8`

# If a match was found (should be!), flag as unstable, print, quit.
if [ -n "${GIT_VERSION}" ]; then
	echo "${GIT_VERSION}-unstable"
	exit 0
fi

# This shouldn't happen, but maybe for old git versions...
echo "$0: failed to describe the git version" >&2
exit 1

