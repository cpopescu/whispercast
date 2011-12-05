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
# Finds GnomeVFS library
#
#  GnomeVFS_INCLUDE_DIR - where to find gnomevfs.h, etc.
#  GnomeVFS_LIBRARIES   - List of libraries when using GnomeVFS.
#  GnomeVFS_FOUND       - True if GnomeVFS found.
#

if (GnomeVFS_INCLUDE_DIR)
  # Already in cache, be silent
  set(GnomeVFS_FIND_QUIETLY TRUE)
endif (GnomeVFS_INCLUDE_DIR)

find_path(GnomeVFS_INCLUDE_DIR libgnomevfs/gnome-vfs.h
  /opt/local/include
  /usr/local/include
  /usr/include
  /opt/local/include/gnome-vfs-2.0
  /usr/local/include/gnome-vfs-2.0
  /usr/include/gnome-vfs-2.0
)

set(GnomeVFS_NAMES gnomevfs-2)
find_library(GnomeVFS_LIBRARY
  NAMES ${GnomeVFS_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (GnomeVFS_INCLUDE_DIR AND GnomeVFS_LIBRARY)
   set(GnomeVFS_FOUND TRUE)
   set( GnomeVFS_LIBRARIES ${GnomeVFS_LIBRARY} )
else (GnomeVFS_INCLUDE_DIR AND GnomeVFS_LIBRARY)
   set(GnomeVFS_FOUND FALSE)
   set(GnomeVFS_LIBRARIES)
endif (GnomeVFS_INCLUDE_DIR AND GnomeVFS_LIBRARY)

if (GnomeVFS_FOUND)
   if (NOT GnomeVFS_FIND_QUIETLY)
      message(STATUS "Found GnomeVFS: ${GnomeVFS_LIBRARY}")
   endif (NOT GnomeVFS_FIND_QUIETLY)
else (GnomeVFS_FOUND)
   if (GnomeVFS_FIND_REQUIRED)
      message(STATUS "Looked for GnomeVFS libraries named ${GnomeVFS_NAMES}.")
      message(STATUS "Include file detected: [${GnomeVFS_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${GnomeVFS_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find GnomeVFS library")
   endif (GnomeVFS_FIND_REQUIRED)
endif (GnomeVFS_FOUND)

mark_as_advanced(
  GnomeVFS_LIBRARY
  GnomeVFS_INCLUDE_DIR
  )
