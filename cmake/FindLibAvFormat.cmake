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

# Finds AvFormat library
#
#  AvFormat_INCLUDE_DIR - where to find avformat.h, etc.
#  AvFormat_LIBRARIES   - List of libraries when using AvFormat.
#  AvFormat_FOUND       - True if AvFormat found.
#

find_path(AvFormat_INCLUDE_DIR1 libavformat/avformat.h
  /opt/local/include
  /usr/local/include
  /usr/include
  )
find_path(AvFormat_INCLUDE_DIR2 ffmpeg/avformat.h
  /opt/local/include
  /usr/local/include
  /usr/include
  )
if ( AvFormat_INCLUDE_DIR1 )
  set (AV_FORMAT_INCLUDE_FILE libavformat/avformat.h)
  set (AvFormat_INCLUDE_DIR ${AvFormat_INCLUDE_DIR1})
endif  ( AvFormat_INCLUDE_DIR1 )
if ( AvFormat_INCLUDE_DIR2 )
  set (AV_FORMAT_INCLUDE_FILE ffmpeg/avformat.h)
  set (AvFormat_INCLUDE_DIR ${AvFormat_INCLUDE_DIR2})
endif  ( AvFormat_INCLUDE_DIR2 )

set(AvFormat_NAMES avformat)
find_library(AvFormat_LIBRARY
  NAMES ${AvFormat_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
  )

if (AvFormat_INCLUDE_DIR AND AvFormat_LIBRARY)
   set(AvFormat_FOUND TRUE)
   set( AvFormat_LIBRARIES ${AvFormat_LIBRARY} )
else (AvFormat_INCLUDE_DIR AND AvFormat_LIBRARY)
   set(AvFormat_FOUND FALSE)
   set(AvFormat_LIBRARIES)
endif (AvFormat_INCLUDE_DIR AND AvFormat_LIBRARY)

if (AvFormat_FOUND)
   if (NOT AvFormat_FIND_QUIETLY)
      message(STATUS "Found AvFormat: ${AvFormat_LIBRARY}")
   endif (NOT AvFormat_FIND_QUIETLY)
else (AvFormat_FOUND)
   if (AvFormat_FIND_REQUIRED)
      message(STATUS "Looked for AvFormat libraries named ${AvFormat_NAMES}.")
      message(STATUS "Include file detected: [${AvFormat_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${AvFormat_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find AvFormat library")
   endif (AvFormat_FIND_REQUIRED)
endif (AvFormat_FOUND)

mark_as_advanced(
  AvFormat_LIBRARY
  AvFormat_INCLUDE_DIR
  )
