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
#include <whisperlib/common/io/file/file_input_stream.h>
#include <whisperlib/net/base/selector.h>
#include <whisperlib/net/base/connection.h>

#include <whisperstreamlib/base/media_file_reader.h>
#include <whisperstreamlib/base/media_info.h>
#include <whisperstreamlib/base/media_info_util.h>
#include <whisperstreamlib/base/consts.h>
#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/f4v/f4v_tag.h>
#include <whisperstreamlib/f4v/atoms/movie/moov_atom.h>
#include <whisperstreamlib/rtp/rtp_sdp.h>
#include <whisperstreamlib/rtp/rtp_util.h>
#include <whisperstreamlib/rtp/rtp_packetization.h>

#include <whisperstreamlib/rtp/rtsp/rtsp_server.h>

#define LOG_TEST LOG(-1)

DEFINE_string(rtsp_media_dir,
              "",
              "Path to media base directory.");
DEFINE_string(rtsp_local_addr,
              "",
              "Address and port for RTSP server");

class FileMediaInterface : public streaming::rtsp::MediaInterface {
private:
  struct Reader {
    Reader(FileMediaInterface* master,
           net::Selector* selector,
           streaming::MediaFileReader* reader,
           streaming::rtp::Broadcaster* broadcaster)
      : selector_(selector),
        reader_(reader),
        broadcaster_(broadcaster),
        next_tag_callback_(NewPermanentCallback(this, &Reader::NextTag)),
        first_tag_ts_(0) {
    }
    ~Reader() {
      selector_->UnregisterAlarm(next_tag_callback_);
      delete reader_;
      delete next_tag_callback_;
    }
    void NextTag() {
      scoped_ref<streaming::Tag> tag;
      streaming::TagReadStatus result = reader_->Read(&tag);
      if ( result != streaming::READ_OK ) {
        master_->NotifyClose(this);
        return;
      }
      CHECK_NOT_NULL(tag.get());
      broadcaster_->HandleTag(tag.get());

      int64 now = timer::TicksMsec();
      if ( first_tag_ts_ == 0 ) {
        first_tag_ts_ = now;
      }
      int64 real_delay = now - first_tag_ts_;
      int64 stream_delay = tag->timestamp_ms();
      int64 delay = stream_delay - real_delay;
      selector_->RegisterAlarm(next_tag_callback_, delay);
    }
    void Play(bool play) {
      if ( play ) {
        selector_->RegisterAlarm(next_tag_callback_, 0);
      } else {
        selector_->UnregisterAlarm(next_tag_callback_);
      }
    }
    FileMediaInterface* master_;
    net::Selector* selector_;
    streaming::MediaFileReader* reader_;
    streaming::rtp::Broadcaster* broadcaster_;
    Closure* next_tag_callback_;
    int64 first_tag_ts_;
  };
  typedef map<streaming::rtp::Broadcaster*, Reader*> ReaderMap;

public:
  FileMediaInterface(net::Selector* selector, const string& basedir)
    : streaming::rtsp::MediaInterface(),
      selector_(selector),
      basedir_(basedir),
      readers_() {
  }
  virtual ~FileMediaInterface() {
    for ( ReaderMap::iterator it = readers_.begin(); it != readers_.end(); ++it ) {
      delete it->second;
      readers_.erase(it);
    }
    readers_.clear();
  }
  void NotifyClose(Reader* reader) {
    LOG_WARNING << "NotifyClose reader";
    //reader->Play(false);
    //Detach(reader->broadcaster_);
  }
  ////////////////////////////////////////////////////////////////////////////
  // streaming::rtsp::MediaInterface methods
  virtual bool Describe(const string& media, MediaInfoCallback* callback) {
    LOG_ERROR << "PERFORMANCE! use a local SDP cache";
    string file = strutil::JoinPaths(basedir_, media);
    streaming::MediaInfo info;
    if ( !streaming::util::ExtractMediaInfoFromFile(file, &info) ) {
      return false;
    }
    callback->Run(&info);
    return true;
  }
  virtual bool Attach(streaming::rtp::Broadcaster* broadcaster,
      const string& media) {
    CHECK(readers_.find(broadcaster) == readers_.end()) << " Multiple Attach";
    string file = strutil::JoinPaths(basedir_, media);
    streaming::MediaFileReader* file_reader = new streaming::MediaFileReader();
    if ( !file_reader->Open(file) ) {
      LOG_ERROR << "Failed to open file: [" << file << "]";
      delete file_reader;
      return false;
    }
    readers_[broadcaster] = new Reader(this, selector_, file_reader, broadcaster);
    return true;
  }
  virtual void Detach(streaming::rtp::Broadcaster* broadcaster) {
    ReaderMap::iterator it = readers_.find(broadcaster);
    if ( it == readers_.end() ) {
      LOG_ERROR << "Cannot Detach, no such Broadcaster";
      return;
    }
    Reader* reader = it->second;
    readers_.erase(it);

    reader->Play(false);
    selector_->DeleteInSelectLoop(reader);
  }
  virtual void Play(streaming::rtp::Broadcaster* broadcaster, bool play) {
    ReaderMap::iterator it = readers_.find(broadcaster);
    CHECK(it != readers_.end());
    Reader* reader = it->second;
    reader->Play(play);
  }
private:
  net::Selector* selector_;
  const string basedir_;
  ReaderMap readers_;
};

class RtspSimpleServer : public app::App {
private:
  net::Selector* selector_;
  FileMediaInterface* media_interface_;
  streaming::rtsp::Server* rtsp_server_;

public:
  RtspSimpleServer(int argc, char **&argv) :
    app::App(argc, argv),
    selector_(NULL),
    media_interface_(NULL),
    rtsp_server_(NULL) {
  }
  virtual ~RtspSimpleServer() {
    CHECK_NULL(selector_);
    CHECK_NULL(media_interface_);
    CHECK_NULL(rtsp_server_);
  }

  virtual int Initialize() {
    common::Init(argc_, argv_);

    if ( !io::IsDir(FLAGS_rtsp_media_dir) ) {
      LOG_ERROR << "Invalid rtsp_media_dir: [" << FLAGS_rtsp_media_dir << "]";
      return -1;
    }
    if ( net::HostPort(FLAGS_rtsp_local_addr.c_str()).port() == 0 ) {
      LOG_ERROR << "Invalid rtsp_local_addr: [" << FLAGS_rtsp_local_addr << "]";
      return -1;
    }

    //////////////////////////////////////////////////////////////////////
    // Create the selector
    selector_ = new net::Selector();

    //////////////////////////////////////////////////////////////////////
    // Create the media interface
    media_interface_ = new FileMediaInterface(selector_, FLAGS_rtsp_media_dir);

    //////////////////////////////////////////////////////////////////////
    // Create the rtsp server
    rtsp_server_ = new streaming::rtsp::Server(selector_,
        net::NetFactory(selector_).CreateAcceptor(net::PROTOCOL_TCP),
        net::HostPort(FLAGS_rtsp_local_addr.c_str()),
        media_interface_);
    if ( !rtsp_server_->Start() ) {
      LOG_ERROR << "Failed to start the RTSP server.";
      return -1;
    }

    return 0;
  }

  virtual void Run() {
    selector_->Loop();
  }
  virtual void Stop() {
    if ( !selector_->IsInSelectThread() ) {
      selector_->RunInSelectLoop(NewCallback(this, &RtspSimpleServer::Stop));
      return;
    }
    /////////////////////////////////////////////////////////////////////
    // Stop RTSP server
    rtsp_server_->Stop();

    /////////////////////////////////////////////////////////////////////
    // Stop selector
    selector_->RunInSelectLoop(
        NewCallback(selector_, &net::Selector::MakeLoopExit));
  }

  virtual void Shutdown() {
    delete rtsp_server_;
    rtsp_server_ = NULL;

    delete media_interface_;
    media_interface_ = NULL;

    delete selector_;
    selector_ = NULL;
  }
};

int main(int argc, char ** argv) {
  return common::Exit(RtspSimpleServer(argc, argv).Main());
}
