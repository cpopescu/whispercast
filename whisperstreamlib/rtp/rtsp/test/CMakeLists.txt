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

ADD_EXECUTABLE(rtsp_simple_server
  rtsp_simple_server.cc)
ADD_DEPENDENCIES(rtsp_simple_server
  whisper_lib
  whisper_streamlib)
TARGET_LINK_LIBRARIES(rtsp_simple_server
  whisper_lib
  whisper_streamlib)
#INSTALL (TARGETS rtsp_simple_server
#         DESTINATION bin)

ADD_EXECUTABLE(rtsp_codec_test
  rtsp_codec_test.cc)
ADD_DEPENDENCIES(rtsp_codec_test
  whisper_lib
  whisper_streamlib)
TARGET_LINK_LIBRARIES(rtsp_codec_test
  whisper_lib
  whisper_streamlib)
ADD_TEST(rtsp_codec_test
  rtsp_codec_test --sample_messages ${CMAKE_CURRENT_SOURCE_DIR}/data/rtsp_sample_messages)

ADD_EXECUTABLE(rtsp_client
  rtsp_client.cc)
ADD_DEPENDENCIES(rtsp_client
  whisper_lib
  whisper_streamlib)
TARGET_LINK_LIBRARIES(rtsp_client
  whisper_lib
  whisper_streamlib)
INSTALL (TARGETS rtsp_client
         DESTINATION bin)
