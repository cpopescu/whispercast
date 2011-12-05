# Copyright (c) 2010, Whispersoft s.r.l.
# All rights reserved.
#

ADD_EXECUTABLE(simple_wget simple_wget.cc)
ADD_DEPENDENCIES(simple_wget  whisper_lib)
TARGET_LINK_LIBRARIES(simple_wget whisper_lib)

ADD_EXECUTABLE(simple_server_step1 simple_server_step1.cc)
ADD_DEPENDENCIES(simple_server_step1  whisper_lib)
TARGET_LINK_LIBRARIES(simple_server_step1 whisper_lib)

ADD_EXECUTABLE(simple_server_step2 simple_server_step2.cc)
ADD_DEPENDENCIES(simple_server_step2  whisper_lib)
TARGET_LINK_LIBRARIES(simple_server_step2 whisper_lib)

ADD_EXECUTABLE(simple_server_step3 simple_server_step3.cc)
ADD_DEPENDENCIES(simple_server_step3  whisper_lib)
TARGET_LINK_LIBRARIES(simple_server_step3 whisper_lib)
