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
# Finds GSM library
#
#  GSM_INCLUDE_DIR - where to find gsm.h, etc.
#  GSM_LIBRARIES   - List of libraries when using GSM.
#  GSM_FOUND       - True if GSM found.
#

if (GSM_INCLUDE_DIR)
  # Already in cache, be silent
  set(GSM_FIND_QUIETLY TRUE)
endif (GSM_INCLUDE_DIR)

find_path(GSM_INCLUDE_DIR gsm/gsm.h
  /opt/local/include
  /usr/local/include
  /usr/include
)

set(GSM_NAMES gsm)
find_library(GSM_LIBRARY
  NAMES ${GSM_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (GSM_INCLUDE_DIR AND GSM_LIBRARY)
   set(GSM_FOUND TRUE)
   set( GSM_LIBRARIES ${GSM_LIBRARY} )
else (GSM_INCLUDE_DIR AND GSM_LIBRARY)
   set(GSM_FOUND FALSE)
   set(GSM_LIBRARIES)
endif (GSM_INCLUDE_DIR AND GSM_LIBRARY)

if (GSM_FOUND)
   if (NOT GSM_FIND_QUIETLY)
      message(STATUS "Found GSM: ${GSM_LIBRARY}")
   endif (NOT GSM_FIND_QUIETLY)
else (GSM_FOUND)
   if (GSM_FIND_REQUIRED)
      message(STATUS "Looked for GSM libraries named ${GSM_NAMES}.")
      message(STATUS "Include file detected: [${GSM_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${GSM_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find GSM library")
   endif (GSM_FIND_REQUIRED)
endif (GSM_FOUND)

mark_as_advanced(
  GSM_LIBRARY
  GSM_INCLUDE_DIR
  )
