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
#include <whisperlib/common/app/app.h>
#include <whisperlib/common/io/ioutil.h>
#include <whisperlib/net/base/selector.h>
#include <whisperlib/net/base/connection.h>

#include <whisperstreamlib/rtp/rtsp/rtsp_simple_client.h>

#define LOG_TEST LOG(INFO)

DEFINE_string(rtsp_url,
              "",
              "Rtsp url, with server ip address, port maybe, and media.");
DEFINE_int32(num_clients,
             1,
             "The number of parallel clients to run.");
DEFINE_bool(loop,
            false,
            "Loop each client.");
DEFINE_string(runtime_ms,
              "5000",
              "Runtime for each client. Use a number, or an interval."
              " e.g.: 5000 or 1000-5000");
DEFINE_string(start_delay_ms,
              "200",
              "Time between consecutive clients start."
              " Use a number, or an interval."
              " e.g.: 5000 or 1000-5000");
DEFINE_string(reconnect_ms,
              "100",
              "Time between close/connect."
              " Use a number, or an interval."
              " e.g.: 5000 or 1000-5000");
DEFINE_int32(stats_printer_interval_ms,
             5000,
             "Print media stats every so milliseconds.");

struct ClientStats {
  uint64 audio_bytes_;
  uint64 audio_frames_;
  uint64 video_bytes_;
  uint64 video_frames_;
  ClientStats()
    : audio_bytes_(0),
      audio_frames_(0),
      video_bytes_(0),
      video_frames_(0) {
  }
  ClientStats& operator=(const ClientStats& other) {
    audio_bytes_ = other.audio_bytes_;
    audio_frames_ = other.audio_frames_;
    video_bytes_ = other.video_bytes_;
    video_frames_ = other.video_frames_;
    return *this;
  }
  ClientStats& operator+=(const ClientStats& other) {
    audio_bytes_ += other.audio_bytes_;
    audio_frames_ += other.audio_frames_;
    video_bytes_ += other.video_bytes_;
    video_frames_ += other.video_frames_;
    return *this;
  }
  string ToString() const {
    ostringstream oss;
    oss << "Media Stats: " << endl
        << " - audio_frames: " << audio_frames_
        << ", audio_bytes: " << audio_bytes_ << endl
        << " - video_frames: " << video_frames_
        << ", video_bytes: " << video_bytes_;
    return oss.str();
  }
};

class SingleClient {
private:
  net::Selector* selector_;

  // client id. To keep track of multiple clients.
  uint32 id_;

  net::HostPort rtsp_addr_;
  string rtsp_media_;
  streaming::rtsp::SimpleClient rtsp_client_;

  ClientStats stats_;

  // callback to Stop(). Used for stopping the client after runtime ms.
  Closure* auto_stop_;
  // callback in parent RtspClient
  Closure* call_on_close_;

public:
  SingleClient(net::Selector* selector, uint32 id, const net::HostPort& addr,
               const string& media, Closure* call_on_close)
    : selector_(selector),
      id_(id),
      rtsp_addr_(addr),
      rtsp_media_(media),
      rtsp_client_(selector_),
      stats_(),
      auto_stop_(NewPermanentCallback(this, &SingleClient::Stop, true)),
      call_on_close_(call_on_close) {
    rtsp_client_.set_sdp_handler(NewPermanentCallback(this, &SingleClient::RtspClientSdpHandler));
    rtsp_client_.set_rtp_handler(NewPermanentCallback(this, &SingleClient::RtspClientRtpHandler));
    rtsp_client_.set_close_handler(NewPermanentCallback(this, &SingleClient::RtspClientCloseHandler));
  }
  virtual ~SingleClient() {
    selector_->UnregisterAlarm(auto_stop_);
    delete auto_stop_;
    auto_stop_ = NULL;
  }

  uint32 id() const {
    return id_;
  }
  const ClientStats& stats() const {
    return stats_;
  }

  void Start(int64 runtime_ms) {
    rtsp_client_.Open(rtsp_addr_, rtsp_media_);
    selector_->RegisterAlarm(auto_stop_, runtime_ms);
  }
  void Stop(bool run_timeout = false) {
    if ( run_timeout ) {
      LOG_INFO << Info() << "Run time exceeded, stopping.. ";
    }
    rtsp_client_.Close();
  }

  void RtspClientRtpHandler(const io::MemoryStream* ms, bool is_audio) {
    //LOG_DEBUG << Info() << "RTP " << (is_audio ? "AUDIO" : "VIDEO") << ": "
    //          << ms->Size() << " bytes";

    // update stats
    if ( is_audio ) {
      stats_.audio_frames_++;
      stats_.audio_bytes_ += ms->Size();
    } else {
      stats_.video_frames_++;
      stats_.video_bytes_ += ms->Size();
    }
  }
  void RtspClientSdpHandler(const streaming::rtp::Sdp* sdp) {
    LOG_INFO << Info() << "SDP: " << endl << sdp->WriteString();
  }
  void RtspClientCloseHandler(bool is_error) {
    selector_->UnregisterAlarm(auto_stop_);
    CHECK_NOT_NULL(call_on_close_)
        << "Multiple calls to RtspClientCloseHandler";
    selector_->RunInSelectLoop(call_on_close_);
    call_on_close_ = NULL;
  }
  string Info() const {
    return strutil::StringPrintf("C%d: ", id_);
  }
};

class RtspClient : public app::App {
public:
  static const int64 kStatsPrinterInterval = 5000; // milliseconds
  struct Interval {
    int64 min_;
    int64 max_;
    Interval() : min_(0), max_(0) {}
    Interval(int64 min, int64 max) : min_(min), max_(max) {}
    bool Read(const string& s) {
      pair<string, string> p = strutil::SplitFirst(s, "-");
      string s_min = strutil::StrTrim(p.first);
      string s_max = strutil::StrTrim(p.second);
      min_ = ::atoll(s_min.c_str());
      max_ = ::atoll(s_max.c_str());
      if ( (min_ == 0 && s_min != "") ||
           (max_ == 0 && s_max != "") ) {
        LOG_ERROR << "Invalid number in interval: [" << s << "]";
        return false;
      }
      if ( max_ == 0 ) {
        max_ = min_;
      }
      if ( min_ > max_ ) {
        LOG_ERROR << "Invalid interval, number order: [" << s << "]";
        return false;
      }
      return true;
    }
    int64 Rand() const {
      if ( min_ == max_ ) {
        return min_;
      }
      return min_ + (::rand() % (max_ - min_));
    }
  };
private:
  net::Selector* selector_;
  net::HostPort rtsp_addr_;
  string rtsp_media_;

  vector<SingleClient*> clients_;
  int32 running_clients_;

  Interval runtime_ms_;
  Interval start_delay_ms_;
  Interval reconnect_ms_;

  // cumulative stats for all the dead clients
  ClientStats dead_stats_;
  Closure* stats_printer_;

  bool stopping_;

public:
  RtspClient(int argc, char **&argv) :
    App::App(argc, argv),
    selector_(NULL),
    rtsp_addr_(),
    rtsp_media_(),
    clients_(),
    running_clients_(0),
    runtime_ms_(),
    start_delay_ms_(),
    reconnect_ms_(),
    dead_stats_(),
    stats_printer_(NULL),
    stopping_(false) {
  }
  virtual ~RtspClient() {
    CHECK_NULL(selector_);
    CHECK_NULL(stats_printer_);
  }

  virtual int Initialize() {
    common::Init(argc_, argv_);

    if ( !runtime_ms_.Read(FLAGS_runtime_ms) ||
         !start_delay_ms_.Read(FLAGS_start_delay_ms) ||
         !reconnect_ms_.Read(FLAGS_reconnect_ms) ) {
      return -1;
    }

    //////////////////////////////////////////////////////////////////////
    // Create the selector
    selector_ = new net::Selector();

    //////////////////////////////////////////////////////////////////////
    // Create the vector of single clients
    if ( FLAGS_rtsp_url == "" ) {
      LOG_ERROR << "missing rtsp_url flag";
      return -1;
    }
    URL url(FLAGS_rtsp_url);
    if ( url.scheme() != "rtsp" ) {
      LOG_ERROR << "rtsp_url must be a \"rtsp://..\" url";
      return -1;
    }
    if ( !url.HostIsIPAddress() ) {
      LOG_ERROR << "rtsp_url must contain IP address instead of server name";
      return -1;
    }
    rtsp_addr_ = net::HostPort(url.host().c_str(), url.IntPort() > 0 ?
        (uint16)url.IntPort() : streaming::rtsp::kDefaultPort);
    rtsp_media_ = url.path().substr(1);

    clients_.resize(FLAGS_num_clients, NULL);
    selector_->RunInSelectLoop(NewCallback(this,
        &RtspClient::StartEachSingleClient, 0));

    //////////////////////////////////////////////////////////////////////
    // Start the stats printer
    stats_printer_ = NewPermanentCallback(this, &RtspClient::StatsPrinter);
    selector_->RegisterAlarm(stats_printer_, FLAGS_stats_printer_interval_ms);

    return 0;
  }

  virtual void Run() {
    selector_->Loop();
  }
  virtual void Stop() {
    if ( !selector_->IsInSelectThread() ) {
      selector_->RunInSelectLoop(NewCallback(this, &RtspClient::Stop));
      return;
    }

    stopping_ = true;

    /////////////////////////////////////////////////////////////////////
    // Stop stats printer
    selector_->UnregisterAlarm(stats_printer_);

    /////////////////////////////////////////////////////////////////////
    // Stop RTSP clients
    for ( uint32 i = 0; i < clients_.size(); i++ ) {
      if ( clients_[i] == NULL ) {
        continue;
      }
      clients_[i]->Stop();
    }

    /////////////////////////////////////////////////////////////////////
    // Stop selector
    MaybeStopSelector();
  }

  virtual void Shutdown() {
    delete stats_printer_;
    stats_printer_ = NULL;
    for ( uint32 i = 0; i < clients_.size(); i++ ) {
      delete clients_[i];
      clients_[i] = NULL;
    }
    delete selector_;
    selector_ = NULL;
  }

  void MaybeStopSelector() {
    if ( (stopping_ || !FLAGS_loop) && running_clients_ == 0 ) {
      selector_->RunInSelectLoop(
          NewCallback(selector_, &net::Selector::MakeLoopExit));
    }
  }

  void StartEachSingleClient(int32 client_index) {
    StartSingleClient(client_index);
    if ( client_index < clients_.size() - 1 && !stopping_ ) {
      selector_->RegisterAlarm(NewCallback(this,
          &RtspClient::StartEachSingleClient, client_index+1),
          start_delay_ms_.Rand());
    }
  }
  void StartSingleClient(int32 client_index) {
    CHECK_NULL(clients_[client_index]);
    clients_[client_index] = new SingleClient(selector_, client_index,
        rtsp_addr_, rtsp_media_,
        NewCallback(this, &RtspClient::SingleClientClosed, client_index));
    clients_[client_index]->Start(runtime_ms_.Rand());
    running_clients_++;
    LOG_TEST << "START "
             << strutil::StringPrintf("%3d", clients_[client_index]->id())
             << " / total running: " << running_clients_;
  }
  void RestartSingleClient(int32 client_index) {
    dead_stats_ += clients_[client_index]->stats();
    delete clients_[client_index];
    clients_[client_index] = NULL;
    StartSingleClient(client_index);
  }
  void SingleClientClosed(int32 client_index) {
    running_clients_--;
    LOG_TEST << "STOP  "
             << strutil::StringPrintf("%3d", clients_[client_index]->id())
             << " / total running: " << running_clients_;

    if ( !stopping_ && FLAGS_loop ) {
      // restart client
      selector_->RegisterAlarm(NewCallback(this,
          &RtspClient::RestartSingleClient, client_index),
          reconnect_ms_.Rand());
      return;
    }

    MaybeStopSelector();
  }

  void StatsPrinter() {
    ClientStats s = dead_stats_;
    for ( uint32 i = 0; i < clients_.size(); i++ ) {
      if ( clients_[i] == NULL ) {
        continue;
      }
      s += clients_[i]->stats();
    }
    LOG_TEST << s.ToString();
    selector_->RegisterAlarm(stats_printer_, FLAGS_stats_printer_interval_ms);
  }
};

int main(int argc, char ** argv) {
  return common::Exit(RtspClient(argc, argv).Main());
}
