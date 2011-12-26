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
######################################################################

project (whispercast)

set(VERSION_MAJOR "0")
set(VERSION_MINOR "1")
set(VERSION_MICRO "1")
set(VERSION_PATCH "0")

cmake_minimum_required(VERSION 2.4)

IF(COMMAND cmake_policy AND ${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} GREATER 4)
  CMAKE_POLICY(SET CMP0002 NEW)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy AND ${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} GREATER 4)

######################################################################

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_SOURCE_DIR}/../cmake ${CMAKE_MODULE_PATH})

include(../cmake/common.cmake)

find_package(WhisperLib REQUIRED)
find_package(WhisperStreamLib REQUIRED)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../)
include_directories(${whisperlib_INCLUDE_DIR})
include_directories(${whisperstreamlib_INCLUDE_DIR})

####################

link_libraries(${COMMON_LINK_LIBRARIES})

####################

ADD_EXECUTABLE(whispercast
  media_mapper.cc
  stream_request.cc
  stream_server.cc)

ADD_DEPENDENCIES(whispercast
  whisper_lib
  whisper_streamlib)

TARGET_LINK_LIBRARIES(whispercast
  ${whisperlib_LIBRARY}
  ${whisperstreamlib_LIBRARY})

####################

ADD_EXECUTABLE(rtmp_client
  rtmp_client_app.cc)

ADD_DEPENDENCIES(rtmp_client
  whisper_lib
  whisper_streamlib)

TARGET_LINK_LIBRARIES(rtmp_client
  ${whisperlib_LIBRARY}
  ${whisperstreamlib_LIBRARY})

####################

ADD_CUSTOM_TARGET(whispercast_test)
ADD_DEPENDENCIES(whispercast_test rtmp_client whispercast)
ADD_TEST(whispercast_test ${CMAKE_CURRENT_SOURCE_DIR}/whispercast_test.sh ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

# Generate the ini and start file..
SET(INSTALL_DEPENDENT_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/scripts/whispercast.ini
  ${CMAKE_CURRENT_SOURCE_DIR}/scripts/start_whispercast.sh)
ADD_CUSTOM_COMMAND(
  OUTPUT ${INSTALL_DEPENDENT_FILES}
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_local_install.py ${CMAKE_INSTALL_PREFIX} ${CMAKE_CURRENT_SOURCE_DIR}/scripts
  DEPENDS scripts/prepare_local_install.py)
ADD_CUSTOM_TARGET(install_config_files ALL
  DEPENDS ${INSTALL_DEPENDENT_FILES})

######################################################################
#
# INSTALL stuff
#

install(TARGETS whispercast
  DESTINATION bin)
install(TARGETS rtmp_client
  DESTINATION bin)

install(FILES
  etc/config/extension2content_type
  DESTINATION "${CMAKE_INSTALL_PREFIX_ETC}/config")

#install(FILES
#  www/index.html
#  www/menu.html
#  www/welcome.html
#  www/example1.html
#  www/example2.html
#  www/example3.html
#  www/example4.html
#  www/example5.html
#  www/example6.html
#  www/example7.html
#  www/example8.html
#  www/example9.html
#  www/example10.html
#  www/example11.html
#  www/example12.html
#  www/whispercast_logo_m.png
#  www/whispercast_logo.png
#  www/whispercast_logo_s.png
#  DESTINATION "${CMAKE_INSTALL_PREFIX_WWW}")
#install(FILES
#  www/css/whispercast.css
#  DESTINATION "${CMAKE_INSTALL_PREFIX_WWW}/css")
#install(FILES
#  www/js/swfobject.js
#  www/js/whispercast.js
#  DESTINATION "${CMAKE_INSTALL_PREFIX_WWW}/js")
#install(FILES
#  www/js/player/player.js
#  DESTINATION "${CMAKE_INSTALL_PREFIX_WWW}/js/player")

#install(FILES
#  www/swf/player.swf
#  www/swf/LiveStreams.swf
#  DESTINATION "${CMAKE_INSTALL_PREFIX_WWW}/swf")

#find_path(EXAMPLE_FLV_PATH example1.flv
#  ${CMAKE_CURRENT_SOURCE_DIR}/www/media
#)

#if ( EXAMPLE_FLV_PATH )
#  install(FILES
#    www/media/example1.flv
#    www/media/example2.flv
#    www/media/example3.flv
#    www/media/example4.flv
#    DESTINATION "${CMAKE_INSTALL_PREFIX_WWW}/media")
#endif ( EXAMPLE_FLV_PATH )
