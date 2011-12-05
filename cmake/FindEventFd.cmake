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
# Finds out if epoll exists - we look for epoll.h

if (EVENTFD_INCLUDE_DIR)
  # Already in cache, be silent
  set(EVENTFD_FIND_QUIETLY TRUE)
endif (EVENTFD_INCLUDE_DIR)

find_path(EVENTFD_INCLUDE_DIR sys/eventfd.h
  /opt/local/include
  /usr/local/include
  /usr/include
)

if (EVENTFD_INCLUDE_DIR)
  set(EVENTFD_FOUND TRUE)
endif (EVENTFD_INCLUDE_DIR)

if (EVENTFD_FOUND)
  if (NOT EVENTFD_FIND_QUIETLY)
    message(STATUS "Found epoll: ${EVENTFD_INCLUDE_DIR}")
  endif (NOT EVENTFD_FIND_QUIETLY)
else (EVENTFD_FOUND)
  if (EVENTFD_FIND_REQUIRED)
    message(FATAL_ERROR "=========> Could NOT find eventfd - required")
  endif (EVENTFD_FIND_REQUIRED)
endif (EVENTFD_FOUND)

mark_as_advanced(
  EVENTFD_FOUND
  EVENTFD_INCLUDE_DIR
  )
