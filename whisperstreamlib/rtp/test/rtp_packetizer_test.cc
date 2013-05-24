// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// * Neither the name of Whispersoft s.r.l. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Cosmin Tudorache

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/scoped_ptr.h>
#include <whisperlib/common/io/ioutil.h>
#include <whisperlib/common/io/file/file_input_stream.h>

#include <whisperlib/net/base/selector.h>
#include <whisperlib/net/base/udp_connection.h>

#include <whisperstreamlib/base/media_info.h>
#include <whisperstreamlib/base/media_info_util.h>
#include <whisperstreamlib/f4v/f4v_tag.h>
#include <whisperstreamlib/f4v/f4v_encoder.h>
#include <whisperstreamlib/f4v/f4v_decoder.h>
#include <whisperstreamlib/f4v/f4v_file_reader.h>
#include <whisperstreamlib/f4v/atoms/movie/moov_atom.h>

#include <whisperstreamlib/rtp/rtp_sdp.h>
#include <whisperstreamlib/rtp/rtp_util.h>
#include <whisperstreamlib/rtp/rtp_packetization.h>

#define LOG_TEST LOG(INFO)

DEFINE_string(f4v_path,
              "",
              "Path to a f4v file to packetize.");

using namespace std;

int main(int argc, char ** argv) {
  common::Init(argc, argv);
  LOG_TEST << "Test start";

  streaming::f4v::FileReader file_reader;
  if ( !file_reader.Open(FLAGS_f4v_path) ) {
    LOG_ERROR << "Failed to open file: [" << FLAGS_f4v_path << "]";
    common::Exit(1);
  }

  streaming::rtp::H264Packetizer video_packetizer(false);
  streaming::rtp::Mp4aPacketizer audio_packetizer(false);

  uint32 frame_count = 0;
  uint32 packet_count = 0;
  while (true) {
    scoped_ref<streaming::f4v::Tag> tag;
    streaming::f4v::TagDecodeStatus result = file_reader.Read(&tag);
    if ( result == streaming::f4v::TAG_DECODE_ERROR ) {
      LOG_ERROR << "Error decoding next tag.";
      common::Exit(1);
    }
    if ( result == streaming::f4v::TAG_DECODE_NO_DATA ) {
      LOG_INFO << "EOF reached";
      break;
    }
    CHECK_EQ(result, streaming::f4v::TAG_DECODE_SUCCESS);

    if ( tag->is_atom() &&
         tag->atom()->type() == streaming::f4v::ATOM_MOOV ) {
      const streaming::f4v::MoovAtom& moov =
          static_cast<const streaming::f4v::MoovAtom&>(*tag->atom());

      streaming::MediaInfo info;
      bool success = streaming::util::ExtractMediaInfoFromMoov(moov,
          file_reader.decoder().frames(), &info);
      CHECK(success);

      streaming::rtp::Sdp sdp;
      streaming::rtp::util::ExtractSdpFromMediaInfo(3333, 4444, info, &sdp);

      LOG_TEST << "Sdp: " << sdp.ToString();
    }
    if ( tag->is_frame() ) {
      streaming::f4v::Frame* frame = tag->frame();
      LOG_DEBUG << "#" << frame_count << ": " << frame->ToString();

      vector<io::MemoryStream*> packets;
      if ( frame->header().type() == streaming::f4v::FrameHeader::AUDIO_FRAME ) {
        if ( !audio_packetizer.Packetize(frame->data(), &packets) ) {
          LOG_ERROR << "Bad audio data in stream.";
          break;
        }
      } else if ( frame->header().type() == streaming::f4v::FrameHeader::VIDEO_FRAME ) {
        if ( !video_packetizer.Packetize(frame->data(), &packets) ) {
          LOG_ERROR << "Bad video data in stream.";
          break;
        }
      } else {
        LOG_ERROR << "Don't know how to packetize frame type: " << frame->header().type_name();
        continue;
      }

      for ( uint32 i = 0; i < packets.size(); i++ ) {
        LOG_DEBUG << "Rtp Packet: " << packets[i]->DumpContentHex();
        packet_count++;
        delete packets[i];
      }
      packets.clear();
      frame_count++;
    }
  }
  LOG_TEST << "Everything looks OK. #" << frame_count << " frames processed"
              " => #" << packet_count << " rtp packets created";

  LOG_TEST << "Test end";
  common::Exit(0);
}
