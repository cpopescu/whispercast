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
# Finds Glog library
#
#  Glog_INCLUDE_DIR - where to find glog.h, etc.
#  Glog_LIBRARIES   - List of libraries when using Glog.
#  Glog_FOUND       - True if Glog found.
#

if (Glog_INCLUDE_DIR)
  # Already in cache, be silent
  set(Glog_FIND_QUIETLY TRUE)
endif (Glog_INCLUDE_DIR)

find_path(Glog_INCLUDE_DIR glog/logging.h
  /opt/local/include
  /usr/local/include
  /usr/include
)

set(Glog_NAMES glog)
find_library(Glog_LIBRARY
  NAMES ${Glog_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (Glog_INCLUDE_DIR AND Glog_LIBRARY)
   set(Glog_FOUND TRUE)
   set( Glog_LIBRARIES ${Glog_LIBRARY} )
else (Glog_INCLUDE_DIR AND Glog_LIBRARY)
   set(Glog_FOUND FALSE)
   set(Glog_LIBRARIES)
endif (Glog_INCLUDE_DIR AND Glog_LIBRARY)

if (Glog_FOUND)
   if (NOT Glog_FIND_QUIETLY)
      message(STATUS "Found Glog: ${Glog_LIBRARY}")
   endif (NOT Glog_FIND_QUIETLY)
else (Glog_FOUND)
   if (Glog_FIND_REQUIRED)
      message(STATUS "Looked for Glog libraries named ${Glog_NAMES}.")
      message(STATUS "Include file detected: [${Glog_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${Glog_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find Glog library")
   endif (Glog_FIND_REQUIRED)
endif (Glog_FOUND)

mark_as_advanced(
  Glog_LIBRARY
  Glog_INCLUDE_DIR
  )
