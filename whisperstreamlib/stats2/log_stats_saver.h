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

#ifndef __MEDIA_STATS2_LOG_STATS_SAVER_H__
#define __MEDIA_STATS2_LOG_STATS_SAVER_H__

#include <string>
#include <whisperlib/common/io/buffer/io_memory_stream.h>
#include <whisperlib/common/io/logio/logio.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperstreamlib/stats2/stats_saver.h>

// A statistics saver that uses logio to writes stats.
// The stats are saved under a given directory in several files.
// The log structure is:
//   <log_dir>/<file_base>0
//   <log_dir>/<file_base>1
//   <log_dir>/<file_base>2
//   ...
// The more stats are saved, the more files are created.
// Every file is composed of blocks, every block contains records.
// Every "MediaStats" is serialized and saved as a record.
// (see common/io/logio/logio.h :: LogWriter :: file :: block :: record).

namespace streaming {
class LogStatsSaver : public StatsSaver {
 public:
  // Just for statistics logging:
  //   server_id: a server identifier - one per instance
  //   server_instance: basically the start time of the server
  // LogWriter params:
  //   log_dir: absolute path to the log folder
  //   file_base: base name for log files
  LogStatsSaver(const char* log_dir,
                const char* file_base);
  virtual ~LogStatsSaver();

  //////////////////////////////////////////////////////////////////////

  virtual void Save(const MediaStatEvent* event);

 private:
  io::LogWriter writer_;

  DISALLOW_EVIL_CONSTRUCTORS(LogStatsSaver);
};
}

#endif   // __MEDIA_STATS2_LOG_STATS_SAVER_H__
