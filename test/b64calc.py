#!/usr/bin/python

######
# b64calc: test buffer and overflow calculation
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

import base64

def b64calc(s):
	# slen = length of input string
	slen = len(s)
	
	# blen = length of buffer to allocate to fit the string
	blen = ((((slen) + 2) / 3) * 4) + 1
	
	# mlen = max length of string that can fit the buffer
	mlen = ((blen / 4) * 3)
	
	# encode string
	enc = base64.b64encode(s)
	
	# elen = actual length of base64 encoded string
	elen = len(enc)
	
	print "slen = %i // blen = %i // elen = %i // mlen = %i" % \
		(slen, blen, elen, mlen)
	
	if (slen > mlen):
		print "ERROR: STRING IS LONGER THAN MAX STRING: %i > %i" % \
		(slen, mlen)
	if (elen >= blen):
		print "ERROR: ENC STRING DOES NOT FIT IN BUFFER: %i >= %i" % \
		(elen, blen)


for x in xrange(1,64):
	b64calc("\xFF" * x)

