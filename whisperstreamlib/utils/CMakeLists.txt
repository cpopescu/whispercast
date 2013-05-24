project (whispersoft)

ADD_EXECUTABLE(media_file_printer
  media_file_printer.cc)
ADD_DEPENDENCIES(media_file_printer
  whisper_streamlib
  whisper_lib)
TARGET_LINK_LIBRARIES(media_file_printer
  whisper_lib
  whisper_streamlib)

install(TARGETS media_file_printer
  DESTINATION bin)


ADD_EXECUTABLE(media_file_converter
  media_file_converter.cc)
ADD_DEPENDENCIES(media_file_converter
  whisper_streamlib
  whisper_lib)
TARGET_LINK_LIBRARIES(media_file_converter
  whisper_lib
  whisper_streamlib)

install(TARGETS media_file_converter
  DESTINATION bin)
