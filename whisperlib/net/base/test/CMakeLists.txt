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

ADD_EXECUTABLE(selectable_filereader_test selectable_filereader_test.cc)
ADD_DEPENDENCIES(selectable_filereader_test whisper_lib)
TARGET_LINK_LIBRARIES(selectable_filereader_test whisper_lib)


#
# DAMN - did not convince this sucker to run correctly ..
#
# ADD_TEST(selectable_filereader_test
#  selectable_filereader_test
#     "--cmd_path=${CMAKE_CURRENT_SOURCE_DIR}/selectable_filereader_test.sh"
#     "--expected_bytes=27059")


ADD_EXECUTABLE(address_test address_test.cc)
ADD_DEPENDENCIES(address_test whisper_lib)
TARGET_LINK_LIBRARIES(address_test whisper_lib)
ADD_TEST(address_test address_test)

ADD_EXECUTABLE(selector_test selector_test.cc)
ADD_DEPENDENCIES(selector_test whisper_lib)
TARGET_LINK_LIBRARIES(selector_test whisper_lib)
ADD_TEST(selector_test selector_test)
ADD_TEST(selector_ssl_test selector_test 
         "--ssl_key=${CMAKE_CURRENT_SOURCE_DIR}/../ssl.test.key" 
         "--ssl_certificate=${CMAKE_CURRENT_SOURCE_DIR}/../ssl.test.cer")

ADD_EXECUTABLE(udp_connection_test udp_connection_test.cc)
ADD_DEPENDENCIES(udp_connection_test whisper_lib)
TARGET_LINK_LIBRARIES(udp_connection_test whisper_lib)
ADD_TEST(udp_connection_test udp_connection_test
         --num_clients 100
         --num_packets_per_client 10)

ADD_EXECUTABLE(dns_resolver_test dns_resolver_test.cc)
ADD_DEPENDENCIES(dns_resolver_test whisper_lib)
TARGET_LINK_LIBRARIES(dns_resolver_test whisper_lib)
ADD_TEST(dns_resolver_test dns_resolver_test)
         