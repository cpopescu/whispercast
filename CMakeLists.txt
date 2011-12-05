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
#
# Main cmake configuration file - we set a bunch of things here .
#
# To build the project(s), run in this directory:
#    cd build
#    cmake ..
#    make
#
# To build in Debug mode:
#    cd build_debug
#    cmake .. -DCMAKE_BUILD_TYPE=Debug
#    make
#
# Specify the install dir with:
#    cmake .. -DINSTALL_PREFIX=<prefix_dir> ...
#
# Specify enable all rpc tests (some may take long to compile):
#    cmake .. -DRPC_TESTS=All ...
# or
#    cmake .. -DRPC_TESTS=Some ...
#
# To enable encoding libraries and a lot of dependencies:
#    cmake .. -DWITH_GSTREAMER=1 ...
#
#
# You can continue with testing:
#    make test
# And installation of the binaries:
#    make install
#
# To change the install dir:
#    cmake .. -DINSTALL_PREFIX=/your/choice
#
cmake_minimum_required(VERSION 2.4)

enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_SOURCE_DIR}/../cmake ${CMAKE_MODULE_PATH})

include(cmake/common.cmake)

install(DIRECTORY DESTINATION "${CMAKE_INSTALL_PREFIX_ETC}")
install(DIRECTORY DESTINATION "${CMAKE_INSTALL_PREFIX_ETC}/ini")
install(DIRECTORY DESTINATION "${CMAKE_INSTALL_PREFIX_ETC}/config")
install(DIRECTORY DESTINATION "${CMAKE_INSTALL_PREFIX_ETC}/services")
install(DIRECTORY DESTINATION "${CMAKE_INSTALL_PREFIX_ETC}/scripts")
install(DIRECTORY DESTINATION "${CMAKE_INSTALL_PREFIX_VAR}")
install(DIRECTORY DESTINATION "${CMAKE_INSTALL_PREFIX_VAR}/log")
install(DIRECTORY DESTINATION "${CMAKE_INSTALL_PREFIX_VAR}/data")
install(DIRECTORY DESTINATION "${CMAKE_INSTALL_PREFIX_VAR}/state")
install(DIRECTORY DESTINATION "${CMAKE_INSTALL_PREFIX_SCRIPTS}")
install(DIRECTORY DESTINATION "${CMAKE_INSTALL_PREFIX_WWW}")

install(FILES
  scripts/shflags
  scripts/shell
  scripts/start_
  scripts/start
  scripts/stop
  scripts/restart
  scripts/create
  DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
  PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

install(FILES
  scripts/admin/Files.Create
  scripts/admin/Files.Destroy
  scripts/admin/Files.StartExport
  scripts/admin/Files.StopExport
  DESTINATION ${CMAKE_INSTALL_PREFIX_SCRIPTS}/shell/admin
  PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

install(FILES
  scripts/alarm.py
  DESTINATION ${CMAKE_INSTALL_PREFIX_SCRIPTS}/python
  PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

ADD_SUBDIRECTORY (whisperlib)
SET(RPC_PARSER_TARGET rpc_parser)

if (APPLE)
  SET(__MAC_OS "")
  SET(UNINITIALIZED "")
endif (APPLE)

if (APPLE)
  SET(LIB_EXT dylib)
else (APPLE)
  SET(LIB_EXT so)
ENDIF(APPLE)

SET(whisperlib_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/whisperlib/libwhisper_lib.${LIB_EXT})
SET(whisperlib_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/whisperlib)
ADD_SUBDIRECTORY (whisperstreamlib)
SET(whisperstreamlib_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/whisperstreamlib/libwhisper_streamlib.${LIB_EXT})
SET(whisperstreamlib_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/whisperstreamlib)

if (NOT APPLE)
  if (WITH_GSTREAMER)
    ADD_SUBDIRECTORY (whispermedialib)
  endif (WITH_GSTREAMER)
endif (NOT APPLE)
SET(whispermedialib_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/whispermedialib/libwhisper_medialib.${LIB_EXT})
SET(whispermedialib_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/whispermedialib)

ADD_SUBDIRECTORY (whispercast)
ADD_SUBDIRECTORY (private)

######################################################################3
## CPack stuff

set(CPACK_GENERATOR "STGZ")
set(CPACK_PACKAGING_INSTALL_PREFIX $ENV{HOME})
set(CPACK_PACKAGE_NAME whispercast)
set(CPACK_PACKAGE_VENDOR whispersoft)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "whisperCast streaming server")
set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_MICRO ${VERSION_MICRO})
set(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Whispersoft (http://www.whispercast.org)")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libglib2.0-0,libxml2,liboil0.3,libasound2,libx11-6,libxvmc1,libjpeg62,libpng12-0,liblame0,libid3tag0,libfaac0,libfaad0,libraw1394-8,libiec61883-0,libdv4,libavc1394-0,libogg0,libgnomevfs2-0,libsm6,libpango1.0-0,libtheora0,libvorbis0a,libgtk2.0-0,libesd0,libflac8,libspeex1,libgl1-mesa-glx,libgl1-mesa-dri,libbz2-1.0,liba52-0.7.4,libmpeg2-4,libgsm1,libaudio2,libx264-57,libssl0.9.8,libneon27,libfftw3-3,libmad0,libgoogle-perftools0,libbeecrypt6,libaio1,libmysqlclient15off,libicu38")

include(CPack)
