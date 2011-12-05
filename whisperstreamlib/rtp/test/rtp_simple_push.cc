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
#include <whisperlib/common/base/timer.h>
#include <whisperlib/common/io/ioutil.h>
#include <whisperlib/common/io/file/file_input_stream.h>
#include <whisperlib/net/base/selector.h>

#include <whisperstreamlib/base/media_file_reader.h>
#include <whisperstreamlib/base/media_info.h>
#include <whisperstreamlib/base/media_info_util.h>
#include <whisperstreamlib/base/consts.h>
#include <whisperstreamlib/rtp/rtp_sdp.h>
#include <whisperstreamlib/rtp/rtp_util.h>
#include <whisperstreamlib/rtp/rtp_udp_sender.h>
#include <whisperstreamlib/rtp/rtp_broadcaster.h>

#define LOG_TEST LOG(-1)

DEFINE_string(rtp_media_path,
              "",
              "Path to a f4v file for streaming.");
DEFINE_string(rtp_remote_addr,
              "",
              "Destination address for UDP unicast.");
DEFINE_string(rtp_local_addr,
              "",
              "Local IP address, used in SDP's originator address");
DEFINE_string(rtp_sdp_path,
              "",
              "Path to the sdp file where we write stream description.");

using namespace std;

class RtpPusher {
public:
  RtpPusher(net::Selector* selector,
            const net::HostPort& remote_addr)
    : selector_(selector),
      udp_sender_(selector),
      broadcaster_(&udp_sender_, NewPermanentCallback(this,
          &RtpPusher::BroadcasterEosHandler)),
      file_reader_(),
      first_tag_ts_(0),
      next_tag_callback_(NewPermanentCallback(this, &RtpPusher::NextTag)) {
    udp_sender_.SetDestination(remote_addr, true); // audio destination
    udp_sender_.SetDestination(net::HostPort(remote_addr.ip_object().ipv4(),
        remote_addr.port() + 2), false); // video destination
  }
  virtual ~RtpPusher() {
    delete next_tag_callback_;
    next_tag_callback_ = NULL;
  }
  bool OpenFile(const std::string& filename) {
    streaming::MediaInfo media_info;
    if ( !streaming::util::ExtractMediaInfoFromFile(filename, &media_info) ) {
      LOG_ERROR << "ExtractMediaInfoFromFile failed, file: [" << filename << "]";
      return false;
    }
    broadcaster_.SetMediaInfo(media_info);

    streaming::rtp::Sdp sdp;
    streaming::rtp::util::ExtractSdpFromMediaInfo(
        udp_sender_.udp_audio_dst().port(),
        udp_sender_.udp_video_dst().port(),
        media_info,
        &sdp);
    sdp.set_session_name("rtp_simple_push test");
    sdp.set_local_addr(net::IpAddress(FLAGS_rtp_local_addr));
    sdp.set_remote_addr(udp_sender_.udp_audio_dst().ip_object());
    sdp.WriteFile(FLAGS_rtp_sdp_path);

    if ( !file_reader_.Open(filename) ) {
      LOG_ERROR << "Failed to open file: [" << filename << "]";
      return false;
    }
    // disable composed tags
    file_reader_.splitter()->set_max_composition_tag_time_ms(0);
    return true;
  }
  void Start() {
    if( !udp_sender_.Initialize() ) {
      LOG_ERROR << "Failed to initialize UDP sender";
      Stop();
      return;
    }
    LOG_TEST << "Started broadcaster: " << broadcaster_.ToString();
    selector_->RunInSelectLoop(next_tag_callback_);
  }
  void Stop() {
    LOG_TEST << "Stop, stopping selector..";
    selector_->MakeLoopExit();
  }
  void NextTag() {
    scoped_ref<streaming::Tag> tag;
    streaming::TagReadStatus result = file_reader_.Read(&tag);
    if ( result == streaming::READ_EOF ) {
      LOG_INFO << "EOF reached";
      Stop();
      return;
    }
    if ( result != streaming::READ_OK ) {
      LOG_ERROR << "Error decoding next tag.";
      Stop();
      return;
    }
    CHECK_EQ(result, streaming::READ_OK);
    CHECK_NOT_NULL(tag.get());
    broadcaster_.HandleTag(tag.get());

    // schedule NextTag() call
    int64 now = timer::TicksMsec();
    if ( first_tag_ts_ == 0 ) {
      first_tag_ts_ = now;
    }

    int64 real_delay = now - first_tag_ts_;
    int64 stream_delay = tag->timestamp_ms();
    int64 delay = stream_delay - real_delay;

    selector_->RegisterAlarm(next_tag_callback_, delay);
  }
  void BroadcasterEosHandler(bool is_error) {
    // EOF is handled by NextTag. But other broadcaster errors will be
    // reported here.
    LOG_ERROR << "BroadcasterEosHandler is_error: "
              << std::boolalpha << is_error;
    Stop();
  }

private:
  net::Selector* selector_;
  streaming::rtp::UdpSender udp_sender_;
  streaming::rtp::Broadcaster broadcaster_;
  streaming::MediaFileReader file_reader_;

  // for flow control
  int64 first_tag_ts_;

  Closure* next_tag_callback_;
};

int main(int argc, char ** argv) {
  common::Init(argc, argv);
  LOG_TEST << "Test start";

  net::HostPort remote_addr(FLAGS_rtp_remote_addr.c_str());
  if ( remote_addr.IsInvalid() ) {
    LOG_ERROR << "Invalid remote address: [" << FLAGS_rtp_remote_addr << "]";
    common::Exit(1);
  }

  net::Selector selector;
  RtpPusher pusher(&selector, remote_addr);
  if ( !pusher.OpenFile(FLAGS_rtp_media_path) ) {
    LOG_ERROR << "Failed to read file: [" << FLAGS_rtp_media_path << "]";
    common::Exit(1);
  }
  pusher.Start();

  selector.Loop();

  LOG_TEST << "Test end";
  common::Exit(0);
}
