#!/usr/bin/python
#
# Copyright (c) 2009, Whispersoft s.r.l.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
# * Neither the name of Whispersoft s.r.l. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
import os
import sys

def process_wget_out(f, saved_lines):
    lines = open(f).readlines()
    save_lines = False
    for l in lines:
        if ( l.startswith('---response begin---') or
             l.startswith('---request begin---') ):
            save_lines = True
        elif ( l.startswith('---response end---') or
               l.startswith('---request end---' ) ):
            save_lines = False
        elif save_lines:
            saved_lines.append(l)


def get_headers(f, fout):
    saved_lines = []
    lines = open(f).readlines()
    for url in lines:
        url.strip();
        print "Getting: %s" % url
        os.system("wget -d -o /tmp/hdrs1 -O /tmp/xxx_out '%s'" % url)
        os.unlink("/tmp/xxx_out");
        process_wget_out("/tmp/hdrs1", saved_lines)
    fout.write(''.join(saved_lines))

if __name__ == "__main__":
    fout = open(sys.argv[2], "w")
    get_headers(sys.argv[1], fout)
    fout.close()
