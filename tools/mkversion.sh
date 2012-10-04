#!/bin/sh

######
# mkversion.sh: create a version.h file
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

#!/bin/sh

VERSION_FILE="$1"
VERSION_FALLBACK="$2"

TOOLS_PATH=`dirname $0`
TOOLS_GITVERSION="gitversion.sh"

# Output file must be provided
if [ -z "${VERSION_FILE}" ]; then
	echo "usage: `basename $0` <output-file> [<fallback-version>]" >&2
	exit 1
fi

# Get version from git
VERSION=`${TOOLS_PATH}/${TOOLS_GITVERSION}`

if [ -z "${VERSION}" ]; then
	# No version from git
	if [ -z "${VERSION_FALLBACK}" ]; then
		# No fallback version
		echo "`basename $0`: ERROR: no version info found. Add" \
			"BUILD_VERSION=<version> to your make flags if" \
			"you are building outside a git repository." >&2
		exit 1
	fi
	VERSION="${VERSION_FALLBACK}"
fi

cat > "${VERSION_FILE}.tmp" <<EOF
#ifndef _VERSION_H_
#define _VERSION_H_
#define BUILD_VERSION "${VERSION}"
#endif /* _VERSION_H_ */
EOF

if [ -f "${VERSION_FILE}" ]; then
	if ! diff "${VERSION_FILE}.tmp" "${VERSION_FILE}" >/dev/null; then
		# Update file if they are different
		mv "${VERSION_FILE}.tmp" "${VERSION_FILE}"
	else
		# Remove temp file if it's the same
		rm "${VERSION_FILE}.tmp"
	fi
else
	# Rename file if it doesn't exist
	mv "${VERSION_FILE}.tmp" "${VERSION_FILE}"
fi

exit 0

