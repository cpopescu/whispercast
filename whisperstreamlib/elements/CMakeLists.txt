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

project (whisperstreamlib)


######################################################################

# standard_library -> MODULE

FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/standard_library/auto)
SET(STD_LIB_RPC_INPUT_FILES ${CMAKE_CURRENT_SOURCE_DIR}/standard_library/standard_library.rpc )
SET(STD_LIB_RPC_OUTPUT_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/standard_library/auto/standard_library_types.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/standard_library/auto/standard_library_invokers.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/standard_library/auto/standard_library_wrappers.cc)
ADD_CUSTOM_COMMAND (
  OUTPUT ${STD_LIB_RPC_OUTPUT_FILES}
  COMMAND ${RPC_PARSER_EXE} ${STD_LIB_RPC_INPUT_FILES}
  --cc='fServerTypes="${CMAKE_CURRENT_SOURCE_DIR}/standard_library/auto/standard_library_types", fServerInvokers="${CMAKE_CURRENT_SOURCE_DIR}/standard_library/auto/standard_library_invokers", fClientTypes="${CMAKE_CURRENT_SOURCE_DIR}/standard_library/auto/standard_library_types", fClientWrappers="${CMAKE_CURRENT_SOURCE_DIR}/standard_library/auto/standard_library_wrappers"'
  --php='fTypes="${CMAKE_CURRENT_SOURCE_DIR}/standard_library/auto/standard_library_types.php", fWrappers="${CMAKE_CURRENT_SOURCE_DIR}/standard_library/auto/standard_library_invokers.php"'
  --py='file="${CMAKE_CURRENT_SOURCE_DIR}/standard_library/auto/standard_library.py"'
  DEPENDS ${STD_LIB_RPC_INPUT_FILES} ${RPC_PARSER_TARGET}
  )

####################

ADD_LIBRARY (standard_streaming_elements MODULE
  standard_library/standard_library.cc
  standard_library/aio_file/aio_file_element.cc
  standard_library/dropping/dropping_element.cc
  standard_library/http_client/http_client_element.cc
  standard_library/http_poster/http_poster_element.cc
  standard_library/http_server/http_server_element.cc
  standard_library/keyframe/keyframe_element.cc
  standard_library/lookup/lookup_element.cc
  standard_library/normalizing/normalizing_element.cc
  standard_library/load_balancing/load_balancing_element.cc
  standard_library/policies/policy.cc
  standard_library/policies/failover_policy.cc
  standard_library/remote_resolver/remote_resolver_element.cc
  standard_library/rtmp_publishing/rtmp_publishing_element.cc
  standard_library/saving/saving_element.cc
  standard_library/splitting/splitting_element.cc
  standard_library/stream_renamer/stream_renamer_element.cc
  standard_library/switching/switching_element.cc
  standard_library/timesaving/timesaving_element.cc
  standard_library/simple_authorizer/simple_authorizer.cc
  standard_library/f4v_to_flv_converter/f4v_to_flv_converter_element.cc
  standard_library/redirect/redirecting_element.cc

  ${STD_LIB_RPC_OUTPUT_FILES}
  )

ADD_DEPENDENCIES(standard_streaming_elements
  whisper_lib
  whisper_streamlib)

######################################################################


# command_library -> MODULE

FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/command_library/auto)
SET(COMMAND_LIB_RPC_INPUT_FILES ${CMAKE_CURRENT_SOURCE_DIR}/command_library/command_library.rpc )
SET(COMMAND_LIB_RPC_OUTPUT_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/command_library/auto/command_library_types.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/command_library/auto/command_library_invokers.cc)
ADD_CUSTOM_COMMAND (
  OUTPUT ${COMMAND_LIB_RPC_OUTPUT_FILES}
  COMMAND ${RPC_PARSER_EXE} ${COMMAND_LIB_RPC_INPUT_FILES}
  --cc='fServerTypes="${CMAKE_CURRENT_SOURCE_DIR}/command_library/auto/command_library_types", fServerInvokers="${CMAKE_CURRENT_SOURCE_DIR}/command_library/auto/command_library_invokers"'
  --php='fTypes="${CMAKE_CURRENT_SOURCE_DIR}/command_library/auto/command_library_types.php", fWrappers="${CMAKE_CURRENT_SOURCE_DIR}/command_library/auto/command_library_invokers.php"'
  --py='file="${CMAKE_CURRENT_SOURCE_DIR}/command_library/auto/command_library.py"'
  DEPENDS ${COMMAND_LIB_RPC_INPUT_FILES} ${RPC_PARSER_EXE}
  )

####################

if (NOT APPLE) 
ADD_LIBRARY (command_streaming_elements MODULE
  command_library/command_library.cc
  command_library/command_element.cc
  ${COMMAND_LIB_RPC_OUTPUT_FILES}
  )

ADD_DEPENDENCIES (command_streaming_elements
  whisper_lib
  whisper_streamlib)

#TARGET_LINK_LIBRARIES(command_streaming_elements
#  ${whisperlib_LIBRARY}
# ${whisperstreamlib_LIBRARY}
#)
endif (NOT APPLE)

######################################################################

ADD_EXECUTABLE(http_tag_printer
  standard_library/http_client/http_client_element.cc
  standard_library/http_client/http_client_tag_printer.cc)
ADD_DEPENDENCIES(http_tag_printer
  ${whisperlib_LIBRARY}
  whisper_streamlib)
TARGET_LINK_LIBRARIES(http_tag_printer
  ${whisperlib_LIBRARY}
  whisper_streamlib)

####################

#  ADD_SUBDIRECTORY (test)

####################

ADD_SUBDIRECTORY (util/test)

INSTALL (TARGETS standard_streaming_elements
  DESTINATION modules)


if ( ENABLE_COMMAND_STREAMING_ELEMENTS )
  INSTALL (TARGETS command_streaming_elements
    DESTINATION modules)
endif  ( ENABLE_COMMAND_STREAMING_ELEMENTS )
