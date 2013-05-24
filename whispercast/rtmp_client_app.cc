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
// Author: Catalin Popescu
//
// A simple client that talks with the rtmp server -- not much ..
//
#include <whisperlib/common/app/app.h>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/strutil.h>
#include <whisperlib/common/base/scoped_ptr.h>
#include <whisperlib/common/base/timer.h>
#include <whisperlib/common/base/alarm.h>
#include <whisperlib/common/base/util.h>
#include <whisperlib/common/io/file/file.h>

#include <whisperlib/net/base/selector.h>
#include <whisperlib/net/base/connection.h>
#include <whisperlib/net/url/url.h>
#include <whisperstreamlib/base/consts.h>
#include <whisperstreamlib/rtmp/rtmp_client.h>

#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/flv/flv_file_writer.h>

DEFINE_string(rtmp_url,
              "",
              "The rtmp://.. url to play");
DEFINE_int32(num_connections,
             1,
             "Number of connections for our test");
DEFINE_string(time_between_connections_ms,
              "0",
              "Start a new connection after this time."
              " Use a number, or an interval. e.g.: 5000 or 1000-5000");
DEFINE_string(connection_run_time_ms,
              "600000000",
              "Run a connection for this long."
              " Use a number, or an interval. e.g.: 5000 or 1000-5000");
DEFINE_int32(seek_pos_ms,
             -1,
             "If not negative it issues a seek at the specified position "
             "in milliseconds, after receiving --seek_after_events rtmp "
             "events");
DEFINE_int32(seek_after_ms,
             1000,
             "When to perform the seek if --seek_pos_ms is non-zero. We "
             "do the seek after these milliseconds from startup");
DEFINE_string(dump_video_file,
              "",
              "Dumps the received flv to a file (if specified)");
DEFINE_int32(timing_delta_treshold_ms,
             3000,
             "Log warnings if incoming tags are derailed from realtime by"
             "more than 'timing_delta_treshold_ms'");
DEFINE_bool(loop,
            false,
            "Restart connection on close?");

DEFINE_int32(event_log_level,
             2,
             "The log level for the received events");
DEFINE_int32(tag_log_level,
             4,
             "The log level for the received tags");

//////////////////////////////////////////////////////////////////////

using namespace rtmp;

#define ILOG(level)  LOG(level) << "ID " << id_ << ": "
#define ILOG_INFO    ILOG(INFO)
#define ILOG_WARNING ILOG(WARNING)
#define ILOG_ERROR   ILOG(ERROR)
#define ILOG_FATAL   ILOG(FATAL)

static int32 g_current_connections = 0;

// alarms to start or restart each TestConnection
static vector<util::Alarm*> g_start_alarms;

//////////////////////////////////////////////////////////////////////

static util::Interval g_connection_run_time_ms(0,0);
static util::Interval g_time_between_connections_ms(0,0);

class TestConnection {
  static const int64 kRuntimeEvent = 4;
  static const char* TimeoutIdName(int64 timeout_id) {
    switch ( timeout_id ) {
      CONSIDER(kRuntimeEvent);
    }
    return "UNKNOWN";
  }

 public:
  TestConnection(net::Selector* selector,
                 uint32 id,
                 const string& output_filename)
    : selector_(selector),
      client_(new SimpleClient(selector)),
      id_(id),
      out_file_(true, true),
      first_tag_realtime_ms_(0),
      runtime_alarm_(*selector),
      seek_performed_(false) {
    g_current_connections++;
    ILOG(ERROR) << "New client, total running clients: " << g_current_connections;
    client_->set_event_log_level(FLAGS_event_log_level);
    client_->set_tag_log_level(FLAGS_tag_log_level);
    client_->set_close_handler(NewPermanentCallback(this,
        &TestConnection::ClientCloseHandler));
    client_->set_read_handler(NewPermanentCallback(this,
        &TestConnection::ClientReadHandler));

    if ( !output_filename.empty() &&
         !out_file_.Open(output_filename) ) {
      ILOG_ERROR << "Failed to open out file: [" << output_filename << "]";
      selector_->DeleteInSelectLoop(this);
      return;  // Nothing to do ..
    }

    if ( !client_->Open(FLAGS_rtmp_url) ) {
      ILOG_ERROR << "Failed to open url: [" << FLAGS_rtmp_url << "]";
      selector_->DeleteInSelectLoop(this);
      return;  // Nothing to do ..
    }

    runtime_alarm_.Set(NewPermanentCallback(this,
        &TestConnection::RuntimeAlarm),
        true, g_connection_run_time_ms.Rand(), false, true);
  }
  virtual ~TestConnection() {
    CHECK_GE(g_current_connections, 1);
    g_current_connections--;
    ILOG(ERROR) << "Done client, total running clients: " << g_current_connections;

    delete client_;
    client_ = NULL;

    if ( FLAGS_loop && !selector_->IsExiting() ) {
      g_start_alarms[id_]->ResetTimeout(g_time_between_connections_ms.Rand());
      g_start_alarms[id_]->Start();
    } else if (g_current_connections == 0 && !FLAGS_loop) {
      selector_->MakeLoopExit();
    }
  }
 private:
  void RuntimeAlarm() {
    ILOG_WARNING << "Runtime expired, closing connection...";
    client_->Close();
  }
 protected:
  void ClientReadHandler() {
    while ( true ) {
      scoped_ref<streaming::FlvTag> tag = client_->PopTag();
      if ( tag.get() == NULL ) {
        break;
      }
      if ( out_file_.IsOpen() ) {
        out_file_.Write(tag, tag->timestamp_ms());
      }

      // see how far from realtime is the incoming tag
      const int64 now = timer::TicksMsec();
      if ( first_tag_realtime_ms_ == 0 ) {
        first_tag_realtime_ms_ = now;
      }
      //TODO(cosmin): fix future/past tag reporting.
      //int64 tag_realtime = now - first_tag_realtime_ms_;
      //int64 delta_ms = abs(tag->timestamp_ms() - tag_realtime);
      //if ( delta_ms > FLAGS_timing_delta_treshold_ms ) {
      //  ILOG_WARNING << "Tag in the "
      //               << (tag->timestamp_ms() < tag_realtime ? "past" : "future")
      //               << " by " << delta_ms << " ms"
      //                  ", tag_realtime: " << tag_realtime
      //               << ", tag: " << tag->ToString();
      //}

      if ( !seek_performed_ ) {
        if ( FLAGS_seek_pos_ms >= 0 ) {
          if ( tag->timestamp_ms() > FLAGS_seek_after_ms ) {
            seek_performed_ = true;
            client_->Seek(FLAGS_seek_pos_ms);
          }
        }
      }
    }
  }
  void ClientCloseHandler() {
    ILOG_WARNING << "Client closed, auto deleting..";
    selector_->DeleteInSelectLoop(this);
  }

 private:
  net::Selector * selector_;
  // the rtmp client
  SimpleClient* client_;
  // connection id, just for logging
  const uint32 id_;
  // write received tags to this output file
  streaming::FlvFileWriter out_file_;
  // test incoming tags timestamps, compare with system real time
  int64 first_tag_realtime_ms_;
  // terminate client after runtime milliseconds
  util::Alarm runtime_alarm_;
  // seek performed
  bool seek_performed_;
};

void StartRtmpConnection(net::Selector* selector, uint32 id) {
  new TestConnection(selector, id, FLAGS_num_connections == 1 ?
                                   FLAGS_dump_video_file : string(""));
}

class RtmpClient : public app::App {
public:
  RtmpClient(int &argc, char **&argv):
    App::App(argc, argv) {
    selector_ = NULL;
  }
  virtual ~RtmpClient() {
    CHECK_NULL(selector_);
  }

protected:
  int Initialize() {
    common::Init(argc_, argv_);
    srand ( time(NULL) );

    CHECK_GT(FLAGS_num_connections, 0);

    if ( !g_time_between_connections_ms.Read(
            FLAGS_time_between_connections_ms) ) {
      LOG_ERROR << "Invalid time_between_connections_ms flag: ["
                << FLAGS_time_between_connections_ms << "]";
      return 1;
    }

    if ( !g_connection_run_time_ms.Read(FLAGS_connection_run_time_ms) ) {
      LOG_ERROR << "Invalid connection_run_time_ms flag: ["
                << FLAGS_connection_run_time_ms << "]";
      return 1;
    }

    selector_ = new net::Selector();
    for ( uint32 i = 0; i < FLAGS_num_connections; i++ ) {
      g_start_alarms.push_back(new util::Alarm(*selector_));
    }

    return 0;
  }

  void Run() {
    int64 conn_start_delay_ms = 0;
    for ( uint32 i = 0; i < FLAGS_num_connections; i++ ) {
      g_start_alarms[i]->Set(NewPermanentCallback(&StartRtmpConnection,
          selector_, i), true, conn_start_delay_ms, false, true);
      conn_start_delay_ms += g_time_between_connections_ms.Rand();
    }
    selector_->Loop();
  }
  void Stop() {
    CHECK(!selector_->IsInSelectThread());
    if ( selector_->IsExiting() ) {
      // MakeLoopExit() already called, selector_ is no longer running
      return;
    }
    selector_->RunInSelectLoop(
        NewCallback(this, &RtmpClient::StopInSelectThread));
  }
  void StopInSelectThread() {
    CHECK(selector_->IsInSelectThread());
    for ( uint32 i = 0; i < FLAGS_num_connections; i++ ) {
      g_start_alarms[i]->Stop();
    }
    selector_->MakeLoopExit();
  }
  void Shutdown() {
    CHECK_EQ(g_current_connections, 0);
    for ( uint32 i = 0; i < FLAGS_num_connections; i++ ) {
      delete g_start_alarms[i];
    }
    g_start_alarms.clear();
    delete selector_;
    selector_ = NULL;
  }
private:
  net::Selector* selector_;
};

int main(int argc, char* argv[]) {
  RtmpClient app(argc, argv);
  common::Exit(app.Main());
}
