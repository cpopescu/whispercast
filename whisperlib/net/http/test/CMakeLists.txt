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

ADD_EXECUTABLE(http_header_test http_header_test.cc)
ADD_DEPENDENCIES(http_header_test whisper_lib)
TARGET_LINK_LIBRARIES(http_header_test whisper_lib)
ADD_TEST(http_header_test http_header_test)

ADD_EXECUTABLE(http_request_test http_request_test.cc)
ADD_DEPENDENCIES(http_request_test whisper_lib)
TARGET_LINK_LIBRARIES(http_request_test whisper_lib)
ADD_TEST(http_request_test ${CMAKE_CURRENT_SOURCE_DIR}/http_request_test.sh ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

ADD_EXECUTABLE(http_testserver http_testserver.cc)
ADD_DEPENDENCIES(http_testserver whisper_lib)
TARGET_LINK_LIBRARIES(http_testserver whisper_lib)
ADD_TEST(http_testserver http_testserver)

### TODO: creaate a general test with these utilities
### (i.e. put them in a big script :)

ADD_EXECUTABLE(http_server_test http_server_test.cc)
ADD_DEPENDENCIES(http_server_test whisper_lib)
TARGET_LINK_LIBRARIES(http_server_test whisper_lib)

ADD_EXECUTABLE(http_get_page http_get_page.cc)
ADD_DEPENDENCIES(http_get_page  whisper_lib)
TARGET_LINK_LIBRARIES(http_get_page whisper_lib)

ADD_EXECUTABLE(http_proxy_test http_proxy_test.cc)
ADD_DEPENDENCIES(http_proxy_test whisper_lib)
TARGET_LINK_LIBRARIES(http_proxy_test whisper_lib)

ADD_EXECUTABLE(failsafe_test failsafe_test.cc)
ADD_DEPENDENCIES(failsafe_test whisper_lib)
TARGET_LINK_LIBRARIES(failsafe_test whisper_lib)
