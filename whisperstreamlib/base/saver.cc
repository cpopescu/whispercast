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

#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/date.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/io/ioutil.h>
#include <whisperlib/common/io/file/file.h>
#include "base/saver.h"
#include "flv/flv_tag.h"
#include "aac/aac_tag_splitter.h"
#include "mp3/mp3_frame.h"
#include "flv/flv_tag_splitter.h"
#include "raw/raw_tag_splitter.h"

// TODO(cpopescu): add content-type saving and checking
// TODO(cpopescu): use aio for writing files

//////////////////////////////////////////////////////////////////////

DEFINE_int64(media_saver_max_file_size,
             5000000,
             "We initiate a new file when we have written these "
             "many bytes in the old file");

//////////////////////////////////////////////////////////////////////

#define ILOG(level)  LOG(level) << name() << ": "
#ifdef _DEBUG
#define ILOG_DEBUG   ILOG(INFO)
#else
#define ILOG_DEBUG   if (false) ILOG(INFO)
#endif
#define ILOG_INFO    ILOG(INFO)
#define ILOG_WARNING ILOG(WARNING)
#define ILOG_ERROR   ILOG(ERROR)
#define ILOG_FATAL   ILOG(FATAL)

namespace streaming {

const string Saver::kFilePrefix = "part_";
const string Saver::kFileSuffix = ".part";
const string Saver::kFileRE = "^part_[0-9]\\{15\\}[NC]\\.part$";
const string Saver::kTempFileSuffix = ".tmp";
const string Saver::kTempFileRE = "^part_[0-9]\\{15\\}[NC]\\.part\\.tmp$";
const string Saver::kMediaInfoFile = "media.info";

string Saver::MakeFilename(int64 ts, bool is_new_chunk) {
  return strutil::StringPrintf("%s%015"PRId64"%c%s",
                               kFilePrefix.c_str(),
                               ts,
                               is_new_chunk ? 'N' : 'C',
                               kFileSuffix.c_str());
}
string Saver::MakeFilenameNow(bool is_new_chunk) {
  return MakeFilename(timer::Date::Now(), is_new_chunk);
}
string Saver::MakeFilename(const string& temp_filename) {
  if ( strutil::StrEndsWith(temp_filename, kTempFileSuffix) ) {
    return temp_filename.substr(0,
        temp_filename.size() - kTempFileSuffix.size());
  }
  return temp_filename;
}
string Saver::MakeTempFilename(int64 ts, bool is_new_chunk) {
  return MakeFilename(ts, is_new_chunk) + kTempFileSuffix;
}
string Saver::MakeTempFilenameNow(bool is_new_chunk) {
  return MakeTempFilename(timer::Date::Now(), is_new_chunk);
}

bool Saver::ParseFilename(const string& filename,
                          int64* out_ts,
                          bool* out_is_new_chunk) {
  if ( filename.size() != kFilePrefix.size() + kFileSuffix.size() + 16 ||
       (filename[kFilePrefix.size()+15] != 'N' &&
        filename[kFilePrefix.size()+15] != 'C') ) {
    LOG_ERROR << "Invalid saver filename: [" << filename << "]";
    return false;
  }
  *out_ts = ::atoll(filename.c_str() + kFilePrefix.size());
  if ( out_is_new_chunk != NULL ) {
    *out_is_new_chunk = filename[kFilePrefix.size()+15] == 'N';
  }
  return true;
}

Saver::Saver(const string& name,
             ElementMapper* mapper,
             MediaFormat media_format,
             const string& media_name,
             const string& media_dir,
             StopCallback* stop_callback)
  : name_(name),
    mapper_(mapper),
    media_format_(media_format),
    media_name_(media_name),
    media_dir_(media_dir),
    req_(NULL),
    stop_alarm_(NULL),
    stop_callback_(stop_callback),
    processing_callback_(NewPermanentCallback(this, &Saver::ProcessTag)),
    bootstrapper_(false),
    serializer_(NULL) {
  if ( mapper_ != NULL ) {
    stop_alarm_ = new ::util::Alarm(*mapper->selector());
    stop_alarm_->Set(NewPermanentCallback(this, &Saver::StopSaving), true,
        0, false, false);
  }
}

Saver::~Saver() {
  delete stop_alarm_;
  stop_alarm_ = NULL;

  delete stop_callback_;
  stop_callback_ = NULL;

  if ( IsSaving() ) {
    StopSaving();
  }
  delete processing_callback_;
}

bool Saver::OpenNextFile(bool new_chunk) {
  CloseFile();
  const string filename = strutil::JoinPaths(
      media_dir_, MakeTempFilenameNow(new_chunk));
  if ( !current_file_.Open(filename,
                           io::File::GENERIC_READ_WRITE,
                           io::File::CREATE_ALWAYS) ) {
    ILOG_ERROR << "Cannot open output file: [" << filename << "].";
    return false;
  }
  CHECK(serializer_ != NULL);
  serializer_->Initialize(&buffer_);
  SaveBootstrap();
  return true;
}
void Saver::CloseFile() {
  if ( !current_file_.is_open() ) {
    return;
  }
  serializer_->Finalize(&buffer_);
  Flush();
  const string filename = current_file_.filename();
  current_file_.Close();
  // rename temporary file to final file
  if ( strutil::StrEndsWith(filename, kTempFileSuffix) ) {
    io::Rename(filename, MakeFilename(filename), false);
  }
}
void Saver::RecoverTempFiles() {
  // rename all temp files that were left behind by a previous crash, to their
  // final name
  re::RE tmp_re(kTempFileRE);
  vector<string> tmp_files;
  io::DirList(media_dir_, io::LIST_FILES | io::LIST_RECURSIVE, &tmp_re, &tmp_files);
  LOG_INFO << "Recovering " << tmp_files.size() << " tmp files"
              ", probably from a previous crash.";
  for ( uint32 i = 0; i < tmp_files.size(); i++ ) {
    const string tmp_full_path = strutil::JoinPaths(media_dir_, tmp_files[i]);
    io::Rename(tmp_full_path, MakeFilename(tmp_full_path), false);
  }
}

bool Saver::StartSaving(uint32 duration_sec) {
  if ( IsSaving() ) {
    if ( stop_alarm_ != NULL ) {
      stop_alarm_->Stop();
      if ( duration_sec > 0 ) {
        stop_alarm_->ResetTimeout(duration_sec * 1000LL);
        stop_alarm_->Start();
      }
    }
    return true;
  }
  if ( !io::CreateRecursiveDirs(media_dir_) ) {
    ILOG_ERROR << "Cannot create working directory: [" << media_dir_ << "].";
    return false;
  }

  RecoverTempFiles();

  // NOTE: we can work without a mapper_: we just need to receive the tags
  //       through ProcessTag

  // get media details
  if ( mapper_ != NULL &&
       !mapper_->HasMedia(media_name_) ) {
    ILOG_ERROR << "Cannot start because the associated media ["
               << media_name_ << "] was not found.";
    return false;
  }

  // create serializer
  CHECK_NULL(serializer_);
  serializer_ = CreateSerializer(media_format_);
  if ( serializer_ == NULL ) {
    ILOG_ERROR << "Cannot start because media_format_: "
               << MediaFormatName(media_format_) << " cannot be serialized.";
    return false;
  }

  // open output file
  if ( !OpenNextFile(true) ) {
    return false;
  }

  // add request in mapper
  if ( mapper_ != NULL ) {
    CHECK(req_ == NULL);
    req_ = new streaming::Request();
    req_->mutable_info()->is_internal_ = true;
    if ( !mapper_->AddRequest(media_name_.c_str(),
                              req_,
                              processing_callback_) ) {
      ILOG_INFO << "Cannot AddRequest on media_name_: [" << media_name_ << "]";
      delete req_;
      req_ = NULL;
      io::Rm(current_file_.filename());
      CloseFile();
      return false;
    }
  }

  // create media info file. Containing nothing for now.
  io::Touch(strutil::JoinPaths(media_dir_, kMediaInfoFile));

  // schedule the stop alarm
  if ( stop_alarm_ != NULL && duration_sec > 0 ) {
    stop_alarm_->ResetTimeout(duration_sec * 1000LL);
    stop_alarm_->Start();
  }

  ILOG_INFO << "Start saving [" << media_name_ << "] " << " to: '"
            << current_file_.filename() << "'.";
  return true;
}

void Saver::StopSaving() {
  if ( IsSaving() ) {
    if ( mapper_ != NULL ) {
      CHECK(req_ != NULL);
      mapper_->RemoveRequest(req_, processing_callback_);
      req_ = NULL;
    }
    CHECK_NULL(req_);
    CloseFile();
    delete serializer_;
    serializer_ = NULL;
    ILOG_INFO << "Stopped saving stream: [" << media_name_ << "]";
  }

  if ( stop_callback_ != NULL ) {
    StopCallback* c = stop_callback_;
    stop_callback_ = NULL;
    c->Run(this);
  }
}


void Saver::ProcessTag(const Tag* tag, int64 timestamp_ms) {
  if ( !IsSaving() ) {
    // Happens when someone sends tags in a loop without acknowledging the
    // RemoveRequest (e.g. Bootstrapper, Filtering)
    ILOG_ERROR << "Not saving, ignoring tag: " << tag->ToString();
    return;
  }
  if ( tag->type() == Tag::TYPE_EOS ) {
    ILOG_INFO << "Stopping on EOS";
    StopSaving();
    return;
  }
  // Create a new file (N=NEW) on Metadata
  if ( tag->type() == Tag::TYPE_FLV &&
       static_cast<const FlvTag*>(tag)->body().type() == FLV_FRAMETYPE_METADATA &&
       static_cast<const FlvTag*>(tag)->metadata_body().name().value() == kOnMetaData) {
    if ( !OpenNextFile(true) ) {
      ILOG_ERROR << "Failed to open next file to save, stopping";
      StopSaving();
      return;
    }
  }

  bootstrapper_.ProcessTag(tag, timestamp_ms);

  if ( buffer_.Size() > kDumpBufferSize ) {
    Flush();
  }
  // Go to next file (C=CONTINUE) when current file becomes too large
  if ( current_file_.Size() > FLAGS_media_saver_max_file_size &&
       tag->is_video_tag() && tag->can_resync() ) {
    if ( !OpenNextFile(false) ) {
      ILOG_ERROR << "Failed to open next file to save, stopping";
      StopSaving();
      return;
    }
  }
  serializer_->Serialize(tag, timestamp_ms, &buffer_);
}

void Saver::Flush() {
  CHECK(current_file_.is_open());
  ILOG_DEBUG << "Flushing, file_size: " << current_file_.Size()
            << ", buffer_size: " << buffer_.Size();
  current_file_.Write(buffer_);
  current_file_.Flush();
  CHECK(buffer_.IsEmpty());
}
}
