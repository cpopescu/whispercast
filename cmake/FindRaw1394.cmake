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

# Finds Raw1394 library
#
#  Raw1394_INCLUDE_DIR - where to find raw1394.h, etc.
#  Raw1394_LIBRARIES   - List of libraries when using Raw1394.
#  Raw1394_FOUND       - True if Raw1394 found.
#

if (Raw1394_INCLUDE_DIR)
  # Already in cache, be silent
  set(Raw1394_FIND_QUIETLY TRUE)
endif (Raw1394_INCLUDE_DIR)

find_path(Raw1394_INCLUDE_DIR libraw1394/raw1394.h
  /opt/local/include
  /usr/local/include
  /usr/include
)

set(Raw1394_NAMES raw1394)
find_library(Raw1394_LIBRARY
  NAMES ${Raw1394_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (Raw1394_INCLUDE_DIR AND Raw1394_LIBRARY)
   set(Raw1394_FOUND TRUE)
   set( Raw1394_LIBRARIES ${Raw1394_LIBRARY} )
else (Raw1394_INCLUDE_DIR AND Raw1394_LIBRARY)
   set(Raw1394_FOUND FALSE)
   set(Raw1394_LIBRARIES)
endif (Raw1394_INCLUDE_DIR AND Raw1394_LIBRARY)

if (Raw1394_FOUND)
   if (NOT Raw1394_FIND_QUIETLY)
      message(STATUS "Found Raw1394: ${Raw1394_LIBRARY}")
   endif (NOT Raw1394_FIND_QUIETLY)
else (Raw1394_FOUND)
   if (Raw1394_FIND_REQUIRED)
      message(STATUS "Looked for Raw1394 libraries named ${Raw1394_NAMES}.")
      message(STATUS "Include file detected: [${Raw1394_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${Raw1394_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find Raw1394 library")
   endif (Raw1394_FIND_REQUIRED)
endif (Raw1394_FOUND)

mark_as_advanced(
  Raw1394_LIBRARY
  Raw1394_INCLUDE_DIR
  )
