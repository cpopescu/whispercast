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
# Main cmake configuration file - we set a bunch of things here ..
#
# To build the project(s), run in this directory:
#    cmake .
#    make
#
# To build in Debug mode:
#    cmake . -DCMAKE_BUILD_TYPE=Debug
#    make
#
# Specify the install dir with:
#    cmake -DINSTALL_PREFIX=<prefix_dir> ...
#
# You can continue with testing:
#    make test
# And installation of the binaries:
#    make install
#
######################################################################

project (whisperlib)


set(VERSION_MAJOR "0")
set(VERSION_MINOR "1")
set(VERSION_MICRO "1")
set(VERSION_PATCH "0")

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Generally useful library for Linux.")

cmake_minimum_required(VERSION 2.4)

IF(COMMAND cmake_policy AND ${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} GREATER 6 AND ${CMAKE_PATCH_VERSION} GREATER 3)
  CMAKE_POLICY(SET CMP0011 OLD)
ENDIF(COMMAND cmake_policy AND ${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} GREATER 6 AND ${CMAKE_PATCH_VERSION} GREATER 3)

IF(COMMAND cmake_policy AND ${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} GREATER 4)
  CMAKE_POLICY(SET CMP0002 NEW)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy AND ${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} GREATER 4)


######################################################################

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_SOURCE_DIR}/../cmake ${CMAKE_MODULE_PATH})

find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)
find_package(LibSsl REQUIRED)

if (NOT APPLE)
 find_package(LibAio REQUIRED)
endif (NOT APPLE)
######################################################################

include(../cmake/common.cmake)

######################################################################

include_directories (${CMAKE_CURRENT_SOURCE_DIR})
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../)
link_libraries(${COMMON_LINK_LIBRARIES})
include_directories (${Icu_INCLUDE})

if ( NOT RPC_PARSER_TARGET )
  set(RPC_PARSER_TARGET "")
endif ( NOT RPC_PARSER_TARGET )

######################################################################
#
# whisper_lib
#

ADD_SUBDIRECTORY (common)
ADD_SUBDIRECTORY (net)

ADD_LIBRARY (whisper_lib SHARED
  common/app/app.cc

  common/base/errno.cc
  common/base/system.cc
  common/base/signal_handlers.cc
  common/base/strutil.cc
  common/base/re.cc
  common/base/date.cc
  common/base/timer.cc
  common/base/alarm.cc
  common/base/util.cc

  common/base/third-party/string_util.cc

  common/io/buffer/data_block.cc
  common/io/file/aio_file.cc
  common/io/file/buffer_manager.cc
  common/io/file/fd.cc
  common/io/file/file.cc
  common/io/file/file_output_stream.cc
  common/io/buffer/memory_stream.cc
  common/io/buffer/io_memory_stream.cc
  common/io/file/fd_input_stream.cc
  common/io/file/file_input_stream.cc
  common/io/output_stream.cc
  common/io/ioutil.cc
  common/io/stream_base.cc
  common/io/num_streaming.cc
  common/io/checkpoint/checkpointing.cc
  common/io/checkpoint/state_keeper.cc
  common/io/logio/recordio.cc
  common/io/logio/logio.cc
  common/io/zlib/zlibwrapper.cc

  common/math/third-party/imath.cc
  common/math/wavelets.cc

  common/sync/event.cc
  common/sync/thread.cc
  common/sync/thread_pool.cc
  common/sync/system.cc
  common/sync/process.cc

  net/base/selector_base.cc
  net/base/selector.cc
  net/base/selectable.cc
  net/base/connection.cc
  net/base/selectable_filereader.cc
  net/base/timeouter.cc
  net/base/address.cc
  net/base/udp_connection.cc
  net/base/dns_resolver.cc

  net/http/http_consts.cc
  net/http/http_header.cc
  net/http/http_server_protocol.cc
  net/http/http_client_protocol.cc
  net/http/http_request.cc
  net/http/http_proxy.cc
  net/http/failsafe_http_client.cc

  net/rpc/lib/types/rpc_message.cc
  net/rpc/lib/codec/rpc_encoder.cc
  net/rpc/lib/codec/rpc_decoder.cc
  net/rpc/lib/codec/rpc_codec.cc
  net/rpc/lib/codec/binary/rpc_binary_encoder.cc
  net/rpc/lib/codec/binary/rpc_binary_decoder.cc
  net/rpc/lib/codec/json/rpc_json_decoder.cc
  net/rpc/lib/codec/json/rpc_json_encoder.cc
  net/rpc/lib/server/execution/rpc_execution_pool.cc
  net/rpc/lib/server/execution/rpc_execution_worker.cc
  net/rpc/lib/server/irpc_async_query_executor.cc
  net/rpc/lib/server/rpc_core_types.cc
  net/rpc/lib/server/rpc_server_connection.cc
  net/rpc/lib/server/rpc_services_manager.cc
  net/rpc/lib/server/rpc_server.cc
  net/rpc/lib/server/rpc_http_processor.cc
  net/rpc/lib/server/rpc_http_server.cc
  net/rpc/lib/client/irpc_client_connection.cc
  net/rpc/lib/client/rpc_client_connection_tcp.cc
  net/rpc/lib/client/rpc_client_connection_http.cc
  net/rpc/lib/client/rpc_failsafe_client_connection_http.cc
  net/rpc/lib/client/rpc_service_wrapper.cc
  net/rpc/lib/rpc_constants.cc
  net/rpc/lib/rpc_util.cc

  net/url/third-party/google-url/gurl.cc
  net/url/third-party/google-url/url_canon_etc.cc
  net/url/third-party/google-url/url_canon_fileurl.cc
  net/url/third-party/google-url/url_canon_host.cc
  net/url/third-party/google-url/url_canon_icu.cc
  net/url/third-party/google-url/url_canon_internal.cc
  net/url/third-party/google-url/url_canon_ip.cc
  net/url/third-party/google-url/url_canon_path.cc
  net/url/third-party/google-url/url_canon_pathurl.cc
  net/url/third-party/google-url/url_canon_query.cc
  net/url/third-party/google-url/url_canon_relative.cc
  net/url/third-party/google-url/url_canon_stdurl.cc
  net/url/third-party/google-url/url_parse.cc
  net/url/third-party/google-url/url_parse_file.cc
  net/url/third-party/google-url/url_util.cc

  net/util/base64.cc
  net/util/ip2location.cc
  net/util/ipclassifier.cc
  net/util/resolver.cc
  net/util/third-party/IP2Location.cc
)

if (APPLE)
  SET(RT_LIB "")
else (APPLE)
  SET(RT_LIB "rt")
endif (APPLE)

TARGET_LINK_LIBRARIES(whisper_lib
  pthread
  ${RT_LIB}
  crypto
  z
  ssl
  ${LibAio_LIBRARIES}
  ${Icu_LIBRARIES}
  ${COMMON_LINK_LIBRARIES})

######################################################################
#
# INSTALL
#

## Create the installation directories
#install(DIRECTORY DESTINATION bin)
#install(DIRECTORY DESTINATION lib)
#install(DIRECTORY DESTINATION include)
#install(DIRECTORY DESTINATION "${CMAKE_INSTALL_PREFIX_VAR}/www/js")

install(TARGETS whisper_lib
         DESTINATION lib)

install(FILES
  common/app/app.h
  common/base/callback.h
  common/base/common.h
  common/base/date.h
  common/base/errno.h
  common/base/cache.h
  common/base/free_list.h
  common/base/log.h
  common/base/parse.h
  common/base/re.h
  common/base/ref_counted.h
  common/base/scoped_ptr.h
  common/base/signal_handlers.h
  common/base/strutil.h
  common/base/system.h
  common/base/timer.h
  common/base/types.h
  DESTINATION include/whisperlib/common/base)

install(FILES
  common/base/callback/callback.h
  common/base/callback/callback1.h
  common/base/callback/callback2.h
  common/base/callback/callback3.h
  common/base/callback/closure.h
  common/base/callback/result_callback1.h
  common/base/callback/result_callback2.h
  common/base/callback/result_callback3.h
  common/base/callback/result_closure.h
  DESTINATION include/whisperlib/common/base/callback)

install(FILES
  common/base/third-party/scoped_ptr.h
  common/base/third-party/string_util.h
  DESTINATION include/whisperlib/common/base/third-party)

install(FILES
  common/io/buffer/data_block.h
  common/io/buffer/io_memory_stream.h
  common/io/buffer/memory_stream.h
  DESTINATION include/whisperlib/common/io/buffer)

install(FILES
  common/io/checkpoint/checkpointing.h
  common/io/checkpoint/state_keeper.h
  DESTINATION include/whisperlib/common/io/checkpoint)

install(FILES
  common/io/file/aio_file.h
  common/io/file/fd.h
  common/io/file/fd_input_stream.h
  common/io/file/file.h
  common/io/file/file_input_stream.h
  common/io/file/file_output_stream.h
  DESTINATION include/whisperlib/common/io/file)

install(FILES
  common/io/logio/logio.h
  common/io/logio/recordio.h
  DESTINATION include/whisperlib/common/io/logio)

install(FILES
  common/io/zlib/zlibwrapper.h
  DESTINATION include/whisperlib/common/io/zlib)

install(FILES
  common/io/input_stream.h
  common/io/iomarker.h
  common/io/ioutil.h
  common/io/num_streaming.h
  common/io/output_stream.h
  common/io/seeker.h
  common/io/stream_base.h
  DESTINATION include/whisperlib/common/io)


install(FILES
  common/math/imath.h
  DESTINATION include/whisperlib/common/math)

install(FILES
  common/math/third-party/imath.h
  DESTINATION include/whisperlib/common/math/third-party)

install(FILES
  common/sync/event.h
  common/sync/mutex.h
  common/sync/process.h
  common/sync/producer_consumer_queue.h
  common/sync/system.h
  common/sync/thread.h
  common/sync/thread_pool.h
  DESTINATION include/whisperlib/common/sync)

install(FILES
  net/base/address.h
  net/base/connection.h
  net/base/selectable.h
  net/base/selectable_filereader.h
  net/base/selector.h
  net/base/timeouter.h
  net/base/user_authenticator.h
  DESTINATION include/whisperlib/net/base)

install(FILES
  net/http/http_client_protocol.h
  net/http/http_consts.h
  net/http/http_header.h
  net/http/http_proxy.h
  net/http/http_request.h
  net/http/http_server_protocol.h
  DESTINATION include/whisperlib/net/http)

install(FILES
  net/rpc/lib/client/irpc_client_connection.h
  net/rpc/lib/client/rpc_client_connection_http.h
  net/rpc/lib/client/rpc_client_connection_tcp.h
  net/rpc/lib/client/rpc_service_wrapper.h
  DESTINATION include/whisperlib/net/rpc/lib/client)

install(FILES
  net/rpc/lib/codec/binary/rpc_binary_common.h
  net/rpc/lib/codec/binary/rpc_binary_decoder.h
  net/rpc/lib/codec/binary/rpc_binary_encoder.h
  DESTINATION include/whisperlib/net/rpc/lib/codec/binary)

install(FILES
  net/rpc/lib/codec/json/rpc_json_decoder.h
  net/rpc/lib/codec/json/rpc_json_encoder.h
  DESTINATION include/whisperlib/net/rpc/lib/codec/json)

install(FILES
  net/rpc/lib/codec/rpc_codec.h
  net/rpc/lib/codec/rpc_decoder.h
  net/rpc/lib/codec/rpc_encoder.h
  DESTINATION include/whisperlib/net/rpc/lib/codec)

install(FILES
  net/rpc/lib/log_indent.h
  net/rpc/lib/rpc_constants.h
  net/rpc/lib/rpc_util.h
  net/rpc/lib/rpc_version.h
  DESTINATION include/whisperlib/net/rpc/lib)

install(FILES
  net/rpc/lib/server/execution/rpc_execution_pool.h
  net/rpc/lib/server/execution/rpc_execution_simple.h
  net/rpc/lib/server/execution/rpc_execution_worker.h
  DESTINATION include/whisperlib/net/rpc/lib/server/execution)

install(FILES
  net/rpc/lib/server/irpc_async_query_executor.h
  net/rpc/lib/server/irpc_result_handler.h
  net/rpc/lib/server/rpc_core_types.h
  net/rpc/lib/server/rpc_delegate.h
  net/rpc/lib/server/rpc_http_processor.h
  net/rpc/lib/server/rpc_http_server.h
  net/rpc/lib/server/rpc_server.h
  net/rpc/lib/server/rpc_server_connection.h
  net/rpc/lib/server/rpc_service_invoker.h
  net/rpc/lib/server/rpc_services_manager.h
  DESTINATION include/whisperlib/net/rpc/lib/server)

install(FILES
  net/rpc/lib/types/rpc_all_types.h
  net/rpc/lib/types/rpc_message.h
  DESTINATION include/whisperlib/net/rpc/lib/types)

install(FILES
  net/url/third-party/google-url/gurl.h
  net/url/third-party/google-url/url_canon.h
  net/url/third-party/google-url/url_canon_icu.h
  net/url/third-party/google-url/url_canon_internal.h
  net/url/third-party/google-url/url_canon_internal_file.h
  net/url/third-party/google-url/url_canon_stdstring.h
  net/url/third-party/google-url/url_file.h
  net/url/third-party/google-url/url_parse.h
  net/url/third-party/google-url/url_parse_internal.h
  net/url/third-party/google-url/url_util.h
  DESTINATION include/whisperlib/net/url/third-party/google-url)

install(FILES
  net/url/url.h
  DESTINATION include/whisperlib/net/url)

install(FILES
  net/util/base64.h
  net/util/ip2location.h
  net/util/ipclassifier.h
  net/util/resolver.h
  DESTINATION include/whisperlib/net/util)

install(FILES
  net/util/third-party/IP2Location.h
  DESTINATION include/whisperlib/net/util/third-party)

install(FILES
  net/rpc/js/log.js
  net/rpc/js/rpc_base.js
  net/rpc/js/rpc_connection.js
  net/rpc/js/rpc_json.js
  net/rpc/js/rpc_standard.js
  net/rpc/js/rpc_types.js
  net/rpc/js/auto_forms_log.html
  DESTINATION ${CMAKE_INSTALL_PREFIX_WWW}/js/rpc)

######################################################################


ADD_SUBDIRECTORY (examples)
