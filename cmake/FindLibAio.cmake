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
# Finds LibAio library
#
#  LibAio_INCLUDE_DIR - where to find aio.h
#  LibAio_LIBRARIES   - List of libraries when using aio.
#  LibAio_FOUND       - True if libaio found.


if (LibAio_INCLUDE_DIR)
  # Already in cache, be silent
  set(LibAio_FIND_QUIETLY TRUE)
endif (LibAio_INCLUDE_DIR)

find_path(LibAio_INCLUDE_DIR aio.h
  /opt/local/include
  /usr/local/include
  /usr/include
)

set(LibAio_NAMES aio)
find_library(LibAio_LIBRARY
  NAMES ${LibAio_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (LibAio_INCLUDE_DIR AND LibAio_LIBRARY)
   set(LibAio_FOUND TRUE)
   set( LibAio_LIBRARIES ${LibAio_LIBRARY} )
else (LibAio_INCLUDE_DIR AND LibAio_LIBRARY)
   set(LibAio_FOUND FALSE)
   set(LibAio_LIBRARIES)
endif (LibAio_INCLUDE_DIR AND LibAio_LIBRARY)

if (LibAio_FOUND)
   if (NOT LibAio_FIND_QUIETLY)
      message(STATUS "Found AIO Library: ${LibAio_LIBRARY}")
   endif (NOT LibAio_FIND_QUIETLY)
else (LibAio_FOUND)
   if (LibAio_FIND_REQUIRED)
      message(STATUS "Looked for LibAio libraries named ${LibAio_NAMES}.")
      message(FATAL_ERROR "Could NOT find LibAio library")
   endif (LibAio_FIND_REQUIRED)
endif (LibAio_FOUND)

mark_as_advanced(
  LibAio_LIBRARY
  LibAio_INCLUDE_DIR
  )
