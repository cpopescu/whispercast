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

ADD_EXECUTABLE(producer_consumer_queue_test producer_consumer_queue_test.cc)
ADD_DEPENDENCIES(producer_consumer_queue_test whisper_lib)
TARGET_LINK_LIBRARIES(producer_consumer_queue_test whisper_lib)
ADD_TEST(producer_consumer_queue_test producer_consumer_queue_test)

ADD_EXECUTABLE(system_test system_test.cc)
ADD_DEPENDENCIES(system_test whisper_lib)
TARGET_LINK_LIBRARIES(system_test whisper_lib)
ADD_TEST(system_test system_test)

ADD_EXECUTABLE(process_test process_test.cc)
TARGET_LINK_LIBRARIES(process_test whisper_lib)
ADD_DEPENDENCIES(process_test whisper_lib)
ADD_TEST(process_test process_test "--exe" ${CMAKE_CURRENT_SOURCE_DIR}/process_test.py)
