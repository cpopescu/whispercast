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
# Finds Id3Tag library
#
#  Id3Tag_INCLUDE_DIR - where to find id3tag.h, etc.
#  Id3Tag_LIBRARIES   - List of libraries when using Id3Tag.
#  Id3Tag_FOUND       - True if Id3Tag found.
#

if (Id3Tag_INCLUDE_DIR)
  # Already in cache, be silent
  set(Id3Tag_FIND_QUIETLY TRUE)
endif (Id3Tag_INCLUDE_DIR)

find_path(Id3Tag_INCLUDE_DIR id3tag.h
  /opt/local/include
  /usr/local/include
  /usr/include
)

set(Id3Tag_NAMES id3tag)
find_library(Id3Tag_LIBRARY
  NAMES ${Id3Tag_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (Id3Tag_INCLUDE_DIR AND Id3Tag_LIBRARY)
   set(Id3Tag_FOUND TRUE)
   set( Id3Tag_LIBRARIES ${Id3Tag_LIBRARY} )
else (Id3Tag_INCLUDE_DIR AND Id3Tag_LIBRARY)
   set(Id3Tag_FOUND FALSE)
   set(Id3Tag_LIBRARIES)
endif (Id3Tag_INCLUDE_DIR AND Id3Tag_LIBRARY)

if (Id3Tag_FOUND)
   if (NOT Id3Tag_FIND_QUIETLY)
      message(STATUS "Found Id3Tag: ${Id3Tag_LIBRARY}")
   endif (NOT Id3Tag_FIND_QUIETLY)
else (Id3Tag_FOUND)
   if (Id3Tag_FIND_REQUIRED)
      message(STATUS "Looked for Id3Tag libraries named ${Id3Tag_NAMES}.")
      message(STATUS "Include file detected: [${Id3Tag_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${Id3Tag_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find Id3Tag library")
   endif (Id3Tag_FIND_REQUIRED)
endif (Id3Tag_FOUND)

mark_as_advanced(
  Id3Tag_LIBRARY
  Id3Tag_INCLUDE_DIR
  )
