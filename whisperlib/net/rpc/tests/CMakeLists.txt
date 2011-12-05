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

project (whisperlib)

# This test has a liong compile time
if (RPC_TESTS STREQUAL "All")
  ADD_SUBDIRECTORY (codec)
endif (RPC_TESTS STREQUAL "All")

ADD_SUBDIRECTORY (js)
ADD_SUBDIRECTORY (php)

ADD_SUBDIRECTORY (Client-Server-sample)
ADD_SUBDIRECTORY (Client-Server-performance)



ADD_TEST(rpc_test_performance_conformance_http
  ${CMAKE_CURRENT_SOURCE_DIR}/rpc_test_performance_conformance_http.sh ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
ADD_TEST(rpc_test_performance_conformance_tcp
  ${CMAKE_CURRENT_SOURCE_DIR}/rpc_test_performance_conformance_tcp.sh ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
ADD_TEST(rpc_test_sample_http
  ${CMAKE_CURRENT_SOURCE_DIR}/rpc_test_sample_http.sh ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
ADD_TEST(rpc_test_sample_tcp
  ${CMAKE_CURRENT_SOURCE_DIR}/rpc_test_sample_tcp.sh ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

# This tests are long compile time
#if (RPC_TESTS STREQUAL "All")
#  ADD_TEST(${CMAKE_CURRENT_SOURCE_DIR}/rpc_test_performance_tcp.sh ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
#  ADD_TEST(${CMAKE_CURRENT_SOURCE_DIR}/rpc_test_performance_http.sh ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
#  ADD_TEST(${CMAKE_CURRENT_SOURCE_DIR}/rpc_test_performance_http2.sh ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
#  ADD_TEST(${CMAKE_CURRENT_SOURCE_DIR}/rpc_test_performance_http_simple.sh ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
#endif (RPC_TESTS STREQUAL "All")
