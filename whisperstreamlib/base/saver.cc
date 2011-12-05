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

DEFINE_string(saver_file_prefix,
              "part_",
              "We write saved partial files with this prefix");

// TODO(cpopescu): diferentiate on this based on content type (i.e. smarter :)
DEFINE_string(saver_file_suffix,
              ".part",
              "We write partial files w/ this suffix");

//////////////////////////////////////////////////////////////////////

#define ILOG(level)  LOG(level) << name() << ": "
#define ILOG_DEBUG   ILOG(LDEBUG)
#define ILOG_INFO    ILOG(LINFO)
#define ILOG_WARNING ILOG(LWARNING)
#define ILOG_ERROR   ILOG(LERROR)
#define ILOG_FATAL   ILOG(LFATAL)

namespace streaming {

Saver::Saver(const string& name,
             ElementMapper* mapper,
             Tag::Type media_type,
             const string& media_name,
             const string& media_dir,
             int64 start_time,
             bool started_on_command,
             StopCallback* stop_callback)
  : name_(name),
    mapper_(mapper),
    media_type_(media_type),
    media_name_(media_name),
    media_dir_(media_dir),
    start_time_(start_time),
    started_on_command_(started_on_command),
    req_(NULL),
    stop_callback_(stop_callback),
    processing_callback_(NewPermanentCallback(this, &Saver::ProcessTag)),
    bootstrapper_(false),
    serializer_(NULL),
    source_started_(false) {
}

Saver::~Saver() {
  if ( stop_callback_ != NULL )  {
    delete stop_callback_;
    stop_callback_ = NULL;
  }
  if ( IsSaving() ) {
    StopSaving();
  }
  delete processing_callback_;
}

bool Saver::OpenNextFile(bool new_chunk) {
  CloseFile();
  const string filename = GetNextFileForSaving(new_chunk);
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
  current_file_.Close();
}

bool Saver::StartSaving() {
  CHECK(!IsSaving());
  if ( !io::CreateRecursiveDirs(media_dir_) ) {
    ILOG_ERROR << "Cannot create working directory: [" << media_dir_ << "].";
    return false;
  }

  // NOTE: we can work without a mapper_: we just need to receive the tags
  //       through ProcessTag

  // get media details
  Capabilities caps(media_type_, kDefaultFlavourMask);
  if ( mapper_ != NULL ) {
    if ( !mapper_->HasMedia(media_name_.c_str(), &caps) ) {
      ILOG_ERROR << "Cannot start because the associated media ["
                 << media_name_ << "] was not found.";
      return false;
    }
  }

  // create serializer
  delete serializer_;
  serializer_ = NULL;
  switch ( caps.tag_type_ ) {
    case Tag::TYPE_FLV:
      serializer_ = new streaming::FlvTagSerializer(true);
      break;
    case Tag::TYPE_MP3:
      serializer_ = new streaming::Mp3TagSerializer();
      break;
    case Tag::TYPE_AAC:
      serializer_ = new streaming::AacTagSerializer();
      break;
    case Tag::TYPE_RAW:
      serializer_ = new streaming::RawTagSerializer();
      break;
    default:
      ILOG_ERROR << "Cannot start because the tag type: "
                 << caps.tag_type_name() << " cannot be serialized.";
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
    req_->mutable_info()->internal_id_ = "-saver-" + name_;
    *(req_->mutable_caps()) = caps;
    if ( !mapper_->AddRequest(media_name_.c_str(),
                              req_,
                              processing_callback_) ) {
      ILOG_INFO << "Cannot AddRequest on media_name_: ["
                << media_name_ << "], maybe a tag type mismatch: "
                << req_->caps().tag_type_name();
      delete req_;
      req_ = NULL;
      io::Rm(current_file_.filename());
      CloseFile();
      return false;
    }
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
    serializer_->Finalize(&buffer_);
    Flush();
    current_file_.Close();
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


void Saver::ProcessTag(const Tag* tag) {
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
  // Create a new file on SourceStarted
  /*
  if ( tag->type() == Tag::TYPE_SOURCE_STARTED ) {
    source_started_ = true;
  } else {
    if ( source_started_ ) {
      source_started_ = false;
      if ( !OpenNextFile(true) ) {
        ILOG_ERROR << "Failed to open next file to save, stopping";
        StopSaving();
        return;
      }
    }
  }
  */
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

  bootstrapper_.ProcessTag(tag);

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
  serializer_->Serialize(tag, &buffer_);
}

string Saver::GetNextFileForSaving(bool new_chunk) const {
  return strutil::JoinPaths(media_dir_,
             strutil::StringPrintf("%s%015"PRId64"%c%s",
                                   FLAGS_saver_file_prefix.c_str(),
                                   timer::Date::Now(),
                                   new_chunk ? 'N' : 'C',
                                   FLAGS_saver_file_suffix.c_str()));
}

void Saver::Flush() {
  CHECK(current_file_.is_open());
  ILOG_DEBUG << "Flushing, file_size: " << current_file_.Size()
            << ", buffer_size: " << buffer_.Size();
  current_file_.Write(buffer_);
  current_file_.Flush();
  CHECK(buffer_.IsEmpty());
}

bool Saver::CreateSignalingFile(const string& fname, const string& content) {
  const string signal_file(strutil::NormalizePath(
                               media_dir() + "/" + fname));
  io::File f;
  if ( !f.Open(signal_file.c_str(),
               io::File::GENERIC_READ_WRITE,
               io::File::CREATE_ALWAYS) ) {
    ILOG_ERROR << "Error creating the signal file: [" << signal_file << "]";
    return false;
  }
  return f.Write(content) == content.size();
}
}

//////////////////////////////////////////////////////////////////////

namespace streaming {
int64 GetNextStreamMediaFile(
    const timer::Date& play_time,
    const string& home_dir,
    const string& file_prefix,
    const string& root_media_path,
    vector<string>* files,
    int* last_selected,
    string* crt_media,
    int64* begin_file_timestamp,
    int64* playref_time,
    int64* duration) {
  *begin_file_timestamp = 0;
  *duration = 0;

  timer::Date utc_play_time(play_time.GetTime(), true);
  const string utc_play_time_str(utc_play_time.ToShortString());
  timer::Date utc_start_time(true);
  timer::Date utc_end_time(true);
  bool continuation_file = false;
  re::RE regex_files(string("^") + string(file_prefix) +
                     kWhisperProcFileTermination);

  size_t l = 0;
  size_t r = 0;
  size_t pos_uscore1 = 0;
  size_t pos_uscore2 = 0;
  string crt_file;
  if ( !crt_media->empty() && *last_selected < 0 ) {
    pos_uscore1 = crt_media->rfind('_');
    if ( pos_uscore1 != string::npos &&
         pos_uscore1 + 1 + utc_play_time_str.size() <= crt_media->size() ) {
      string crt_media_time_str = crt_media->substr(
          pos_uscore1 + 1, utc_play_time_str.size());
      re::RE regex(string("^") + file_prefix + crt_media_time_str +
                   "_"
                   "[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]-"
                   "[0-9][0-9][0-9][0-9][0-9][0-9]-[0-9][0-9][0-9]"
                   ".*");
      vector<string> local_files;
      if ( !io::DirList(home_dir + "/", &local_files, false, &regex) ) {
        LOG_ERROR << "io::DirList failed for: [" << home_dir << "]";
        local_files.clear();
      }
      if ( !local_files.empty() ) {
        l = -1;
        pos_uscore1 = local_files[0].rfind('_');
        DCHECK(pos_uscore1 != string::npos) << "Bad file: " << local_files[0];
        pos_uscore2 = local_files[0].rfind('_', pos_uscore1 - 1);
        DCHECK(pos_uscore2 != string::npos) << "Bad file: " << local_files[0];
        continuation_file = true;
        crt_file = local_files[0];
        goto RetryForFile;
      }
    } else {
      crt_media->clear();
    }
  }

ReReadFiles:
  if ( files->empty() ) {
    if ( !io::DirList(home_dir + "/", files, false, &regex_files) ) {
      LOG_ERROR << "Error listing under : " << home_dir;
      return -1;
    }
    // this should be cheap as they come already sorted (most of times)
    sort(files->begin(), files->end());
  }
  // binary search ..
  if ( files->empty() ) {
    LOG_ERROR << "No files under: " << home_dir << " w/ prefix: "
              << file_prefix;
    return -1;
  }
  pos_uscore1 = (*files)[0].rfind('_');
  DCHECK(pos_uscore1 != string::npos) << "Bad file: " << (*files)[0];
  pos_uscore2 = (*files)[0].rfind('_', pos_uscore1 - 1);
  DCHECK(pos_uscore2 != string::npos) << "Bad file: " << (*files)[0];
  if ( *last_selected > 0 ) {
    l = *last_selected;
  } else {
    l = 0;
    r = files->size();
    while ( l + 1 < r ) {
      const size_t mid = (r + l) >> 1;
      if ( (*files)[mid].substr(pos_uscore2 + 1, utc_play_time_str.size())
           <= utc_play_time_str ) {
        l = mid;
      } else {
        r = mid;
      }
    }
  }
  crt_file = (*files)[l];
  // Now l is our candidate - extract the times
RetryForFile:
  if ( !utc_start_time.SetFromShortString(
           crt_file.substr(pos_uscore2 + 1, utc_play_time_str.size()), true) ||
       !utc_end_time.SetFromShortString(
           crt_file.substr(pos_uscore1 + 1, utc_play_time_str.size()), true) ) {
    LOG_ERROR << "Invalid time for file to check: " << crt_file
              << " [" << crt_file.substr(pos_uscore2 + 1, utc_play_time_str.size())
              << "] "
              << " [" << crt_file.substr(pos_uscore1 + 1, utc_play_time_str.size())
              << "]"
              << " -- [" << utc_play_time_str << "] " << utc_play_time_str.size()
              << " -- " << crt_file;
    return -1;
  }
  if ( utc_end_time.GetTime() < utc_start_time.GetTime() ) {
    LOG_ERROR << "Really bad file name: " << crt_file;
    return -1;
  }
  if ( utc_end_time.GetTime() <= utc_play_time.GetTime() ) {
    if ( l == files->size() - 1 ) {
      return -1;
    }
    if ( continuation_file ) {
      continuation_file = false;
      crt_media->clear();
      goto ReReadFiles;
    } else {
      l = l + 1;
      crt_file = (*files)[l];
      goto RetryForFile;
    }
  }
  *last_selected = l;
  LOG_DEBUG << "Choosing: [" << crt_file << "] - @: " << utc_play_time_str;
  *crt_media = root_media_path + "/" + crt_file;
  if ( utc_start_time.GetTime() > utc_play_time.GetTime() ) {
    *begin_file_timestamp = 0;
    *duration = utc_end_time.GetTime() - utc_start_time.GetTime();
    *playref_time = utc_start_time.GetTime();
    return utc_start_time.GetTime() - utc_play_time.GetTime();
  }
  const int64 delta = utc_play_time.GetTime() - utc_start_time.GetTime();
  *begin_file_timestamp = delta;
  *duration = utc_end_time.GetTime() - utc_start_time.GetTime() - delta;
  *playref_time = utc_play_time.GetTime();
  return 0;
}
}
