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

project (whisperstreamlib)

set(VERSION_MAJOR "0")
set(VERSION_MINOR "1")
set(VERSION_MICRO "1")
set(VERSION_PATCH "0")

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Streaming library for Linux.")

cmake_minimum_required(VERSION 2.4)

IF(COMMAND cmake_policy AND ${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} GREATER 4)
  CMAKE_POLICY(SET CMP0002 NEW)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy AND ${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} GREATER 4)

######################################################################

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_SOURCE_DIR}/../cmake ${CMAKE_MODULE_PATH})

include(../cmake/common.cmake)
link_libraries(${COMMON_LINK_LIBRARIES})

find_package(WhisperLib REQUIRED)
find_package(Faad REQUIRED)
find_package(LibAvCodec REQUIRED)
find_package(LibAvUtil REQUIRED)
find_package(LibAvFormat REQUIRED)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../)
include_directories(${whisperlib_INCLUDE_DIR})

add_definitions(-D __AV_CODEC_INCLUDE_FILE="<${AV_CODEC_INCLUDE_FILE}>")
add_definitions(-D __AV_UTIL_INCLUDE_FILE="<${AV_UTIL_INCLUDE_FILE}>")
add_definitions(-D __AV_FORMAT_INCLUDE_FILE="<${AV_FORMAT_INCLUDE_FILE}>")

######################################################################


if ( NOT RPC_PARSER_TARGET )
  set(RPC_PARSER_TARGET "")
endif ( NOT RPC_PARSER_TARGET )

ADD_SUBDIRECTORY (aac)
ADD_SUBDIRECTORY (flv)
ADD_SUBDIRECTORY (mp3)
ADD_SUBDIRECTORY (mts)
ADD_SUBDIRECTORY (libav_mts)
ADD_SUBDIRECTORY (f4v)
ADD_SUBDIRECTORY (internal)
ADD_SUBDIRECTORY (elements)
ADD_SUBDIRECTORY (rtmp)
ADD_SUBDIRECTORY (rtp)
ADD_SUBDIRECTORY (stats2)
ADD_SUBDIRECTORY (utils)


################################################################################
#
# whisper_streamlib
#
FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/base/auto)

SET(REQUEST_RPC_INPUT_FILES ${CMAKE_CURRENT_SOURCE_DIR}/base/request.rpc )
SET(REQUEST_RPC_OUTPUT_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/base/auto/request_types.cc)
ADD_CUSTOM_COMMAND(
  OUTPUT ${REQUEST_RPC_OUTPUT_FILES}
  COMMAND ${RPC_PARSER_EXE} ${REQUEST_RPC_INPUT_FILES}
  --cc='fServerTypes="${CMAKE_CURRENT_SOURCE_DIR}/base/auto/request_types", fServerInvokers="${CMAKE_CURRENT_SOURCE_DIR}/base/auto/request_invokers", includePathCut="${CMAKE_CURRENT_SOURCE_DIR}/", includePathPaste="whisperstreamlib/"'
  --php='fTypes="${CMAKE_CURRENT_SOURCE_DIR}/base/auto/request_types.php"'
  --py='file="${CMAKE_CURRENT_SOURCE_DIR}/base/auto/request.py"'
  DEPENDS ${REQUEST_RPC_INPUT_FILES} ${RPC_PARSER_TARGET}
  )

##########

FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/elements/auto)

SET(FACTORY_RPC_INPUT_FILES ${CMAKE_CURRENT_SOURCE_DIR}/elements/factory.rpc )
SET(FACTORY_RPC_OUTPUT_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/elements/auto/factory_types.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/elements/auto/factory_invokers.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/elements/auto/factory_wrappers.cc)
ADD_CUSTOM_COMMAND(
  OUTPUT ${FACTORY_RPC_OUTPUT_FILES}
  COMMAND ${RPC_PARSER_EXE} ${FACTORY_RPC_INPUT_FILES}
  --cc='fServerTypes="${CMAKE_CURRENT_SOURCE_DIR}/elements/auto/factory_types", fServerInvokers="${CMAKE_CURRENT_SOURCE_DIR}/elements/auto/factory_invokers", includePathCut="${CMAKE_CURRENT_SOURCE_DIR}/", includePathPaste="whisperstreamlib/", fClientTypes="${CMAKE_CURRENT_SOURCE_DIR}/elements/auto/factory_types", fClientWrappers="${CMAKE_CURRENT_SOURCE_DIR}/elements/auto/factory_wrappers"'
  --php='fTypes="${CMAKE_CURRENT_SOURCE_DIR}/elements/auto/factory_types.php", fWrappers="${CMAKE_CURRENT_SOURCE_DIR}/elements/auto/factory_invokers.php"'
  --py='file="${CMAKE_CURRENT_SOURCE_DIR}/elements/auto/factory.py"'
  DEPENDS ${FACTORY_RPC_INPUT_FILES} ${RPC_PARSER_TARGET}
  )

##########

FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/stats2/auto)

SET(STATS2_RPC_INPUT_FILES ${CMAKE_CURRENT_SOURCE_DIR}/stats2/media_stats.rpc)
SET(STATS2_RPC_OUTPUT_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/stats2/auto/media_stats_types.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/stats2/auto/media_stats_invokers.cc)
ADD_CUSTOM_COMMAND(
  OUTPUT ${STATS2_RPC_OUTPUT_FILES}
  COMMAND ${RPC_PARSER_EXE} ${STATS2_RPC_INPUT_FILES}
  --cc='fServerTypes="${CMAKE_CURRENT_SOURCE_DIR}/stats2/auto/media_stats_types", fServerInvokers="${CMAKE_CURRENT_SOURCE_DIR}/stats2/auto/media_stats_invokers", includePathCut="${CMAKE_CURRENT_SOURCE_DIR}/", includePathPaste="whisperstreamlib/"'
  --php='fTypes="${CMAKE_CURRENT_SOURCE_DIR}/stats2/auto/media_stats_types.php", fWrappers="${CMAKE_CURRENT_SOURCE_DIR}/stats2/auto/media_stats_invokers.php"'
  --py='file="${CMAKE_CURRENT_SOURCE_DIR}/stats2/auto/media_stats.py"'
  DEPENDS ${STATS2_RPC_INPUT_FILES} ${RPC_PARSER_TARGET}
)

##########

FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/movie/auto)

SET(AUTO_GENERATED_ATOMS
  ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/movie/auto/blnk_atom.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/movie/auto/chpl_atom.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/movie/auto/dinf_atom.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/movie/auto/dlay_atom.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/movie/auto/drpo_atom.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/movie/auto/drpt_atom.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/movie/auto/edts_atom.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/movie/auto/elts_atom.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/movie/auto/hclr_atom.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/movie/auto/hlit_atom.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/movie/auto/href_atom.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/movie/auto/ilst_atom.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/movie/auto/krok_atom.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/movie/auto/load_atom.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/movie/auto/meta_atom.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/movie/auto/pdin_atom.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/movie/auto/sdtp_atom.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/movie/auto/styl_atom.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/movie/auto/tbox_atom.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/movie/auto/twrp_atom.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/movie/auto/udta_atom.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/movie/auto/uuid_atom.cc)

ADD_CUSTOM_COMMAND(
  OUTPUT ${AUTO_GENERATED_ATOMS}
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/scripts/generate_atoms.py ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/movie/auto/ autogen
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/scripts/generate_atoms.py ${CMAKE_CURRENT_SOURCE_DIR}/f4v/atoms/scripts/atoms.py)

##########


set(WHISPER_STREAM_LIB_SOURCES
  rtmp/objects/amf/amf0_util.cc
  rtmp/objects/amf/amf_util.cc
  rtmp/objects/rtmp_objects.cc
  rtmp/events/rtmp_event.cc
  rtmp/events/rtmp_call.cc
  rtmp/events/rtmp_event_notify.cc
  rtmp/events/rtmp_event_invoke.cc
  rtmp/events/rtmp_event_ping.cc
  rtmp/events/rtmp_header.cc
  rtmp/rtmp_coder.cc
  rtmp/rtmp_consts.cc
  rtmp/rtmp_flags.cc
  rtmp/rtmp_handshake.cc
  rtmp/rtmp_acceptor.cc
  rtmp/rtmp_connection.cc
  rtmp/rtmp_stream.cc
  rtmp/rtmp_play_stream.cc
  rtmp/rtmp_publish_stream.cc
  rtmp/rtmp_util.cc
  rtmp/rtmp_client.cc

  base/consts.cc
  base/tag.cc
  base/callbacks_manager.cc
  base/request.cc
  base/filtering_element.cc
  base/tag_splitter.cc
  base/tag_normalizer.cc
  base/tag_dropper.cc
  base/saver.cc
  base/joiner.cc
  base/bootstrapper.cc
  base/media_file_reader.cc
  base/media_file_writer.cc
  base/media_info.cc
  base/media_info_util.cc
  base/tag_distributor.cc
  base/stream_auth.cc
  base/time_range.cc
  base/exporter.cc
  base/tag_serializer_creator.cc
  base/tag_splitter_creator.cc

  aac/aac_tag_splitter.cc

  flv/flv_tag.cc
  flv/flv_coder.cc
  flv/flv_consts.cc
  flv/flv_joiner.cc
  flv/flv_tag_splitter.cc
  flv/flv_file_writer.cc

  mp3/mp3_frame.cc
  mp3/mp3_tag_splitter.cc
  
  mts/mts_consts.cc
  mts/mts_types.cc
  mts/mts_decoder.cc
  mts/mts_encoder.cc
  
  libav_mts/libav_mts_encoder.cc

  internal/internal_frame.cc
  internal/internal_tag_splitter.cc

  raw/raw_tag_splitter.cc

  f4v/atoms/movie/avc1_atom.cc
  f4v/atoms/movie/avcc_atom.cc
  f4v/atoms/movie/co64_atom.cc
  f4v/atoms/movie/ctts_atom.cc
  f4v/atoms/movie/esds_atom.cc
  f4v/atoms/movie/free_atom.cc
  f4v/atoms/movie/ftyp_atom.cc
  f4v/atoms/movie/hdlr_atom.cc
  f4v/atoms/movie/mdat_atom.cc
  f4v/atoms/movie/mdhd_atom.cc
  f4v/atoms/movie/mdia_atom.cc
  f4v/atoms/movie/minf_atom.cc
  f4v/atoms/movie/moov_atom.cc
  f4v/atoms/movie/mp4a_atom.cc
  f4v/atoms/movie/mvhd_atom.cc
  f4v/atoms/movie/null_atom.cc
  f4v/atoms/movie/smhd_atom.cc
  f4v/atoms/movie/stbl_atom.cc
  f4v/atoms/movie/stco_atom.cc
  f4v/atoms/movie/stsc_atom.cc
  f4v/atoms/movie/stsd_atom.cc
  f4v/atoms/movie/stss_atom.cc
  f4v/atoms/movie/stsz_atom.cc
  f4v/atoms/movie/stts_atom.cc
  f4v/atoms/movie/tkhd_atom.cc
  f4v/atoms/movie/trak_atom.cc
  f4v/atoms/movie/vmhd_atom.cc
  f4v/atoms/movie/wave_atom.cc

  ${AUTO_GENERATED_ATOMS}
  f4v/atoms/base_atom.cc
  f4v/atoms/versioned_atom.cc
  f4v/atoms/container_atom.cc
  f4v/atoms/raw_atom.cc
  f4v/atoms/multi_record_versioned_atom.cc
  f4v/atoms/container_versioned_atom.cc
  f4v/frames/frame.cc
  f4v/frames/header.cc
  f4v/f4v_decoder.cc
  f4v/f4v_encoder.cc
  f4v/f4v_types.cc
  f4v/f4v_tag.cc
  f4v/f4v_tag_splitter.cc
  f4v/f4v_to_flv.cc
  f4v/f4v_file_reader.cc
  f4v/f4v_file_writer.cc
  f4v/f4v_util.cc
  f4v/f4v_joiner.cc

  rtp/rtp_sdp.cc
  rtp/rtp_util.cc
  rtp/rtp_packetization.cc
  rtp/rtp_consts.cc
  rtp/rtp_header.cc
  rtp/rtp_udp_sender.cc
  rtp/rtp_broadcaster.cc

  rtp/rtsp/types/rtsp_header.cc
  rtp/rtsp/types/rtsp_header_field.cc
  rtp/rtsp/types/rtsp_message.cc
  rtp/rtsp/types/rtsp_request.cc
  rtp/rtsp/types/rtsp_response.cc
  rtp/rtsp/types/rtsp_interleaved_frame.cc
  rtp/rtsp/rtsp_common.cc
  rtp/rtsp/rtsp_consts.cc
  rtp/rtsp/rtsp_decoder.cc
  rtp/rtsp/rtsp_encoder.cc
  rtp/rtsp/rtsp_connection.cc
  rtp/rtsp/rtsp_server.cc
  rtp/rtsp/rtsp_processor.cc
  rtp/rtsp/rtsp_server_processor.cc
  rtp/rtsp/rtsp_simple_client.cc
  rtp/rtsp/rtsp_session.cc
  rtp/rtsp/rtsp_element_mapper_media_interface.cc

  elements/factory_based_mapper.cc
  elements/factory.cc
  elements/util/media_date.cc

  stats2/stats_collector.cc
  stats2/stats_keeper.cc
  stats2/log_stats_saver.cc)

ADD_LIBRARY (whisper_streamlib SHARED
  ${WHISPER_STREAM_LIB_SOURCES}

  ###

  ${FACTORY_RPC_OUTPUT_FILES}
  ${REQUEST_RPC_OUTPUT_FILES}
  ${STATS2_RPC_OUTPUT_FILES}
)

TARGET_LINK_LIBRARIES(whisper_streamlib
  dl
#  crypt
  ${Faad_LIBRARIES}
  ${AvCodec_LIBRARIES}
  ${AvFormat_LIBRARIES}
  ${AvUtil_LIBRARIES}
  whisper_lib
)

######################################################################
#
# INSTALL
#

#install(DIRECTORY DESTINATION bin)
#install(DIRECTORY DESTINATION lib)
#install(DIRECTORY DESTINATION modules)
#install(DIRECTORY DESTINATION include)

install(TARGETS whisper_streamlib
         DESTINATION lib)

install(FILES
  aac/aac_tag_splitter.h
  DESTINATION include/whisperstreamlib/aac)

install(FILES
  base/element_mapper.h
  base/bootstrapper.h
  base/callbacks_manager.h
  base/consts.h
  base/element.h
  base/filtering_element.h
  base/joiner.h
  base/request.h
  base/saver.h
  base/tag.h
  base/tag_splitter.h
  DESTINATION include/whisperstreamlib/base)

install(FILES
  elements/element_library.h
  elements/factory.h
  elements/factory_based_mapper.h
  elements/util/media_date.h
  elements/auto/factory_types.h
  elements/auto/factory_invokers.h
  DESTINATION include/whisperstreamlib/elements)

install(FILES
  flv/flv_coder.h
  flv/flv_consts.h
  flv/flv_joiner.h
  flv/flv_tag.h
  flv/flv_tag_splitter.h
  DESTINATION include/whisperstreamlib/flv)

install(FILES
  internal/internal_frame.h
  internal/internal_tag_splitter.h
  DESTINATION include/whisperstreamlib/internal)

install(FILES
  mp3/mp3_frame.h
  mp3/mp3_tag_splitter.h
  DESTINATION include/whisperstreamlib/mp3)

install(FILES
  raw/raw_tag_splitter.h
  DESTINATION include/whisperstreamlib/raw)

install(FILES
  stats2/log_stats_saver.h
  stats2/stats_collector.h
  stats2/stats_saver.h
  DESTINATION include/whisperstreamlib/stats2)

install(FILES
  stats2/auto/media_stats_types.h
  stats2/auto/media_stats_invokers.h
  DESTINATION include/whisperstreamlib/stats2/auto)

install(FILES
  rtmp/events/rtmp_call.h
  rtmp/events/rtmp_event.h
  rtmp/events/rtmp_event_invoke.h
  rtmp/events/rtmp_event_notify.h
  rtmp/events/rtmp_event_ping.h
  rtmp/events/rtmp_header.h
  DESTINATION include/whisperstreamlib/rtmp/events)

install(FILES
  rtmp/objects/amf/amf0_util.h
  rtmp/objects/amf/amf_util.h
  rtmp/objects/rtmp_objects.h
  DESTINATION include/whisperstreamlib/rtmp/objects)

install(FILES
  rtmp/rtmp_util.h
  rtmp/rtmp_coder.h
  rtmp/rtmp_acceptor.h
  rtmp/rtmp_connection.h
  rtmp/rtmp_consts.h
  rtmp/rtmp_handshake.h
  rtmp/rtmp_flags.h
  rtmp/rtmp_client.h
  rtmp/rtmp_stream.h
  rtmp/rtmp_play_stream.h
  rtmp/rtmp_publish_stream.h
  DESTINATION include/whisperstreamlib/rtmp)

FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_SCRIPTS}/php/rpc/auto/whisperstreamlib/base)
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/base/auto/request_types.php
  DESTINATION ${CMAKE_INSTALL_PREFIX_SCRIPTS}/php/rpc/auto/whisperstreamlib/base)
if (NOT APPLE)
  FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_SCRIPTS}/php/rpc/auto/whisperstreamlib/elements/command_library)
  install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/elements/command_library/auto/command_library_invokers.php
    ${CMAKE_CURRENT_SOURCE_DIR}/elements/command_library/auto/command_library_types.php
    DESTINATION ${CMAKE_INSTALL_PREFIX_SCRIPTS}/php/rpc/auto/whisperstreamlib/elements/command_library)
endif (NOT APPLE)

FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_SCRIPTS}/php/rpc/auto/whisperstreamlib/elements)
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/elements/auto/factory_invokers.php
  ${CMAKE_CURRENT_SOURCE_DIR}/elements/auto/factory_types.php
  DESTINATION ${CMAKE_INSTALL_PREFIX_SCRIPTS}/php/rpc/auto/whisperstreamlib/elements)
FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_SCRIPTS}/php/rpc/auto/whisperstreamlib/elements/standard_library)
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/elements/standard_library/auto/standard_library_invokers.php
  ${CMAKE_CURRENT_SOURCE_DIR}/elements/standard_library/auto/standard_library_types.php
  DESTINATION ${CMAKE_INSTALL_PREFIX_SCRIPTS}/php/rpc/auto/whisperstreamlib/elements/standard_library)
FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_SCRIPTS}/php/rpc/auto/whisperstreamlib/stats2)
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/stats2/auto/media_stats_invokers.php
  ${CMAKE_CURRENT_SOURCE_DIR}/stats2/auto/media_stats_types.php
  DESTINATION ${CMAKE_INSTALL_PREFIX_SCRIPTS}/php/rpc/auto/whisperstreamlib/stats2)

if (WITH_ADMIN)
  FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_ADMIN}/rpc/auto/whisperstreamlib/base)
  install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/base/auto/request_types.php
    DESTINATION ${CMAKE_INSTALL_PREFIX_ADMIN}/rpc/auto/whisperstreamlib/base)
  if (NOT APPLE)
    FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_ADMIN}/rpc/auto/whisperstreamlib/elements/command_library)
    install(FILES
      ${CMAKE_CURRENT_SOURCE_DIR}/elements/command_library/auto/command_library_invokers.php
      ${CMAKE_CURRENT_SOURCE_DIR}/elements/command_library/auto/command_library_types.php
      DESTINATION ${CMAKE_INSTALL_PREFIX_ADMIN}/rpc/auto/whisperstreamlib/elements/command_library)
  endif (NOT APPLE)
  FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_ADMIN}/rpc/auto/whisperstreamlib/elements)
  install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/elements/auto/factory_invokers.php
    ${CMAKE_CURRENT_SOURCE_DIR}/elements/auto/factory_types.php
    DESTINATION ${CMAKE_INSTALL_PREFIX_ADMIN}/rpc/auto/whisperstreamlib/elements)
  FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_ADMIN}/rpc/auto/whisperstreamlib/elements/standard_library)
  install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/elements/standard_library/auto/standard_library_invokers.php
    ${CMAKE_CURRENT_SOURCE_DIR}/elements/standard_library/auto/standard_library_types.php
    DESTINATION ${CMAKE_INSTALL_PREFIX_ADMIN}/rpc/auto/whisperstreamlib/elements/standard_library)
  FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_ADMIN}/rpc/auto/whisperstreamlib/stats2)
  install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/stats2/auto/media_stats_invokers.php
    ${CMAKE_CURRENT_SOURCE_DIR}/stats2/auto/media_stats_types.php
    DESTINATION ${CMAKE_INSTALL_PREFIX_ADMIN}/rpc/auto/whisperstreamlib/stats2)
endif (WITH_ADMIN)

FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_SCRIPTS}/python/rpc/auto/whisperstreamlib/base)
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/base/auto/request.py
  DESTINATION ${CMAKE_INSTALL_PREFIX_SCRIPTS}/python/rpc/auto/whisperstreamlib/base)
if (NOT APPLE)
  FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_SCRIPTS}/python/rpc/auto/whisperstreamlib/elements/command_library)
  install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/elements/command_library/auto/command_library.py
    DESTINATION ${CMAKE_INSTALL_PREFIX_SCRIPTS}/python/rpc/auto/whisperstreamlib/elements/command_library)
endif (NOT APPLE)

FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_SCRIPTS}/python/rpc/auto/whisperstreamlib/elements)
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/elements/auto/factory.py
  DESTINATION ${CMAKE_INSTALL_PREFIX_SCRIPTS}/python/rpc/auto/whisperstreamlib/elements)
FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_SCRIPTS}/python/rpc/auto/whisperstreamlib/elements/standard_library)
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/elements/standard_library/auto/standard_library.py
  DESTINATION ${CMAKE_INSTALL_PREFIX_SCRIPTS}/python/rpc/auto/whisperstreamlib/elements/standard_library)
FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_SCRIPTS}/python/rpc/auto/whisperstreamlib/stats2)
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/stats2/auto/media_stats.py
  DESTINATION ${CMAKE_INSTALL_PREFIX_SCRIPTS}/python/rpc/auto/whisperstreamlib/stats2)

