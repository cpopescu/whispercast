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

#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <algorithm>
#include <ext/functional> // for select2nd

#include <whisperlib/common/io/ioutil.h>
#include <whisperlib/common/base/timer.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/alarm.h>

#include <whisperstreamlib/base/tag_distributor.h>
#include <whisperstreamlib/elements/standard_library/aio_file/aio_file_element.h>

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

class AioFileReadingStruct : public ElementController {
 public:
  AioFileReadingStruct(net::Selector* selector,
                       streaming::Request* req,
                       const string& filename,
                       const string& filename_relative_to_element,
                       const string& filekey,
                       const string& element_name,
                       int64 file_size,
                       bool disable_pause,
                       bool disable_seek,
                       bool disable_duration,
                       io::AioManager* aio_manager,
                       io::BufferManager* const buf_manager,
                       int fd,
                       TagSplitter* splitter,
                       streaming::ProcessingCallback* callback,
                       Callback1<AioFileReadingStruct*>* notify_closed)
      : selector_(selector),
        req_(req),
        filename_(filename),
        filename_relative_to_element_(filename_relative_to_element),
        filekey_(filekey),
        element_name_(element_name),
        disable_pause_(disable_pause),
        disable_seek_(disable_seek),
        disable_duration_(disable_duration),
        file_size_(file_size),
        aio_manager_(aio_manager),
        buf_manager_(buf_manager),
        buffer_(NULL),
        aio_req_(new io::AioManager::Request(fd, 0, NULL, 0, selector_,
            NewPermanentCallback(
                this, &AioFileReadingStruct::AioOperationCompleted))),
        splitter_(splitter),
        buf_(),
        pos_(0),
        read_skip_(0),
        seek_pos_ms_(-1),
        seek_offset_(-1),
        bootstrapping_(true),
        bootstrapper_(true),
        callback_(callback),
        is_orphaned_(false),
        in_io_operation_(false),
        in_data_processing_(false),
        in_tag_processing_(false),
        pause_count_(0),
        is_eof_(false),
        should_close_(false),
        unpause_alarm_(*selector),
        full_callback_(NewPermanentCallback(this,
            &AioFileReadingStruct::ProcessAioOperation)),
        notify_closed_(notify_closed) {
    unpause_alarm_.Set(NewPermanentCallback(this,
        &AioFileReadingStruct::ProcessDataResume), true, 0, false, false);
    req_->set_controller(this);
  }
  virtual ~AioFileReadingStruct() {
    CHECK_NULL(aio_req_->buffer_);
    CHECK_NOT_NULL(aio_req_->closure_);
    delete aio_req_->closure_;
    aio_req_->closure_ = NULL;
    delete aio_req_;

    CHECK_NULL(req_->controller());

    delete splitter_;
    delete full_callback_;
  }
  string name() const { return "[" + element_name_ + " - " + filename_ + "]"; }

  bool in_io_operation() const { return in_io_operation_; }
  bool in_data_processing() const { return in_data_processing_; }
  bool in_tag_processing() const { return in_tag_processing_; }

  const Request* req() const { return req_; }

  void set_is_orphaned(bool is_orphaned) { is_orphaned_ = is_orphaned; }
  void set_should_close(bool should_close) { should_close_ = should_close; }

  string GetCurrentDataKey() {
    return strutil::StringPrintf("%s@%"PRId64"", filekey_.c_str(), pos_);
  }
  // Controller interface
  bool SupportsPause() const {
    return !disable_pause_;
  }
  bool DisabledPause() const {
    return disable_pause_;
  }
  bool SupportsSeek() const {
    return !disable_seek_;
  }
  bool DisabledSeek() const {
    return disable_seek_;
  }

  bool DisabledDuration() const {
    return disable_duration_;
  }

  bool SupportsPreprocessing() const {
    return true;
  }

  int64 Offset() const {
    return req_->mutable_serving_info()->offset_;
  }
  int64 Size() const {
    return req_->mutable_serving_info()->size_;
  }

  // Pause the element (pause true - > stop, false -> restart)
  virtual bool Pause(bool pause) {
    if ( pause ) {
      ++pause_count_;
      if ( pause_count_ > 1 ) {
        return true;
      }
    } else {
      CHECK_GT(pause_count_, 0);
      --pause_count_;
      if ( pause_count_ > 0 ) {
        return true;
      }
    }
    VLOG(8) << (pause ? " PAUSE " : "GO");

    if ( pause_count_ == 0 &&
         !in_tag_processing_ &&
         !in_io_operation_ ) {
      unpause_alarm_.Start();
    }
    return true;
  }
  // Seek in file. It just sets the seek position. The actual seek is initiated
  // by InitiateAioRequest()
  virtual bool Seek(int64 seek_pos_ms) {
    if ( disable_seek_ ) {
      ILOG_ERROR << "Cannot seek to: " << seek_pos_ms << ", disable_seek_: true";
      return false;
    }
    seek_pos_ms += req_->info().media_origin_pos_ms_;

    if ( cue_points_.get() != NULL ) {
      int found = cue_points_->GetCueForTime(seek_pos_ms);
      if ( found >= 0 ) {
        seek_pos_ms_ = seek_pos_ms;
        seek_offset_ = cue_points_->cue_points()[found].second;
        ILOG_DEBUG << "Seeking to: " << cue_points_->cue_points()[found].first
                  << ", offset: " << cue_points_->cue_points()[found].second
                  << ", requested: " << seek_pos_ms;

        if ( !in_data_processing_ && !in_io_operation_ ) {
          InitiateAioRequest(false);
        }
        return true;
      }
      return false;
    }

    seek_pos_ms_ = seek_pos_ms;
    seek_offset_ = 0;
    return true;
  }

  bool InitiateAioRequest(bool registering) {
    CHECK(!in_io_operation_);
    if (aio_req_->buffer_ != NULL) {
      return true;  // will get called..
    }
    CHECK_NULL(buffer_);

    // if seek is pending:
    //   if we have cue points, then do the seek
    //   else: go ahead with current read, wait for cue points,
    //         ProcessData drops media tags
    if ( seek_offset_ >= 0 ) {
      DCHECK(seek_pos_ms_ >= 0);
      // IMPORTANT - need to align the offset to the block alignment
      size_t block_size = buf_manager_->size();
      read_skip_ = seek_offset_ % block_size;
      pos_ = ((seek_offset_ / block_size) * block_size);
      ILOG_DEBUG << "Advancing to cue point "
                << " @" << pos_
                << " + " << read_skip_
                << " seek offset: " << seek_offset_;
      seek_offset_ = -1;
      buf_.Clear();
    }

    aio_req_->offset_ = pos_;
    aio_req_->errno_ = 0;
    aio_req_->result_ = 0;

    buffer_ = buf_manager_->GetBuffer(GetCurrentDataKey());
    if ( buffer_ == NULL ) {
      ILOG_ERROR << "Error allocating a buffer .. ";
      if ( !registering ) {
        StopElement(true);
      }
      return false;
    }
    aio_req_->buffer_ = buffer_->data_;
    aio_req_->size_ = buffer_->data_capacity_;

    io::BufferManager::Buffer::State state =
        buffer_->BeginUsage(full_callback_);
    if ( state == io::BufferManager::Buffer::IN_LOOKUP ) {
      // someone else is doing the read. We have nothing to do but wait.
      // When the Buffer is full with data, full_callback_ is called.
      in_io_operation_ = true;
      return true;
    }
    if ( state == io::BufferManager::Buffer::VALID_DATA ) {
      // This Buffer is already full with data (from a previous read).
      // Just use the data.
      aio_req_->result_  = buffer_->data_size();
      // Need to decouple this..
      selector_->RunInSelectLoop(full_callback_);
      // to prevent frs deletion while ProcessAioOperation() call is pending
      in_io_operation_ = true;
      return true;
    }
    // We have to read data from file, and fill in Buffer.
    CHECK_EQ(state, io::BufferManager::Buffer::NEW);
    in_io_operation_ = true;
    aio_manager_->Read(aio_req_);
    return true;
  }
  void AioOperationCompleted() {
    CHECK(in_io_operation_);
    // We completed the AIO operation - we have the buffer - need to inform
    // the BufferManager
    if ( aio_req_->errno_ == 0 ) {
      ILOG_DEBUG << "Read completed for: " << buffer_->data_key()
                 << " -> " << aio_req_->result_
                 << " vs: " << aio_req_->size_;
      buffer_->MarkValidData(aio_req_->result_);
    } else {
      ILOG_ERROR << "Error in aio operation"
                    ", err: " << aio_req_->errno_ << "[ "
                 << GetSystemErrorDescription(aio_req_->errno_) << "]";
      buffer_->MarkValidData(0);
    }
    ProcessAioOperation();
  }

  void ProcessAioOperation() {
    CHECK(in_io_operation_);
    in_io_operation_ = false;

    // Deal w/ orphaned requests..
    if ( is_orphaned_ ) {
      ClearFileOperation();
      return;
    }
    if ( should_close_ ) {
      ILOG_INFO << "Stopping element, because should_close";
      StopElement(true);
      return;
    }

    // If Seek requested then discard previous read and perform the seek
    if ( seek_offset_ >= 0 ) {
      DCHECK(seek_pos_ms_ >= 0);
      buffer_->EndUsage(full_callback_);
      aio_req_->buffer_ = NULL;
      buffer_ = NULL;
      InitiateAioRequest(false); // does the seek
      return;
    }
    if ( buffer_->data_size() == 0 ) {
      // Bad stuff - bailing out - process whatever is in buf_
      buffer_->EndUsage(full_callback_);
      aio_req_->buffer_ = NULL;
      buffer_ = NULL;
      is_eof_ = true;
      ProcessData();
      return;
    }
    CHECK(buf_.IsEmpty() || read_skip_ == 0);
    CHECK(aio_req_->buffer_ == buffer_->data_);
    CHECK_GT(buffer_->use_count_, 0);
    buf_.Write(buffer_->data_, buffer_->data_size());
    pos_ += buffer_->data_size();
    is_eof_ = buffer_->data_size() < aio_req_->size_;
    buffer_->EndUsage(full_callback_);
    aio_req_->buffer_ = NULL;
    buffer_ = NULL;
    if ( read_skip_ > 0 ) {
      CHECK_LE(read_skip_, buf_.Size());
      buf_.Skip(read_skip_);
      read_skip_ = 0;
    }
    ProcessData();
  }
  void ProcessData() {
    CHECK(!in_io_operation_);
    CHECK(!in_tag_processing_);

    in_data_processing_ = true;
    while ( true ) {
      if ( is_orphaned_ ) {
        ClearFileOperation();
        in_data_processing_ = false;
        return;
      }

      int64 timestamp_ms;
      scoped_ref<Tag> tag;
      TagReadStatus err = splitter_->GetNextTag(
          &buf_, &tag, &timestamp_ms, is_eof_);

      if ( err == streaming::READ_CORRUPTED_FAIL ||
           err == streaming::READ_OVERSIZED_TAG ||
           err == streaming::READ_EOF ||
           err == streaming::READ_UNKNOWN ) {
        if ( err != streaming::READ_EOF ) {
          ILOG_ERROR << "Error reading next tag, media element: ["
                     << filename_ << "], result: " << err << "("
                     << TagReadStatusName(err) << ")";
        }
        ILOG_INFO << "Stopping element on err: "
                  << streaming::TagReadStatusName(err);
        StopElement(err != streaming::READ_EOF);

        in_data_processing_ = false;
        return;
      }

      if ( err == streaming::READ_NO_DATA ) {
        break;
      }
      if ( err == streaming::READ_SKIP ) {
        continue;
      }

      if ( err != READ_OK ) {
        ILOG_ERROR << "Error reading next tag, result: " << err << "("
                   << TagReadStatusName(err) << ")";
        continue;
      }

      // Store the cue points
      if ( tag->type() == Tag::TYPE_CUE_POINT ) {
        cue_points_ = static_cast<CuePointTag*>(tag.get());
        continue;
      }

      if ( tag->type() == Tag::TYPE_BOS ) {
        DCHECK(bootstrapping_);

        // If an initial seek was requested then stop and do the seek
        if ( req_->info().media_origin_pos_ms_ > 0 ||
            req_->info().seek_pos_ms_ >= 0 ) {
          if ( !Seek(req_->info().seek_pos_ms_) ) {
            StopElement(false);

            in_data_processing_ = false;
            return;
          }
          break;
        }

        // End bootstrapping now and proceed
        EndBootstrapping(timestamp_ms, false);
        continue;
      }

      // If a seek operation is pending then stop and do the seek
      if ( seek_offset_ >= 0 ) {
        break;
      }
      // If a seek operation is complete then send SeekPerformedTag when done
      if ( seek_pos_ms_ >= 0 ) {
        if (timestamp_ms < seek_pos_ms_) {
          // Eat the tags while we're still bootstrapping
          if ( bootstrapping_ ) {
            bootstrapper_.ProcessTag(tag.get(), timestamp_ms);
            continue;
          }
          // Eat the tags that are too early
          continue;
        }

        if ( bootstrapping_ ) {
          // End bootstrapping now, clearing any media bootstrap we might have
          bootstrapper_.ClearMediaBootstrap();
          //... this will also send SeekPerformed
          EndBootstrapping(timestamp_ms, (req_->info().seek_pos_ms_ >= 0));
        } else {
            // Send SeekPerformedTag now
            SendTag(scoped_ref<Tag>(
                new SeekPerformedTag(0,kDefaultFlavourMask)).get(),
                  timestamp_ms - req_->info().media_origin_pos_ms_);
        }

        seek_pos_ms_ = -1;
      } else {
        // Eat the tags while we're still bootstrapping
        if ( bootstrapping_ ) {
          bootstrapper_.ProcessTag(tag.get(), timestamp_ms);
          continue;
        }
      }

      // Check the limit
      if ( req_->info().limit_ms_ >= 0 ) {
        if ( timestamp_ms >=
            (req_->info().media_origin_pos_ms_ + req_->info().limit_ms_) ) {
          ILOG_INFO << "Stopping element on limit: " << timestamp_ms
              << " >= "
              << (req_->info().media_origin_pos_ms_ + req_->info().limit_ms_);
          StopElement(false);

          in_data_processing_ = false;
          return;
        }
      }

      SendTag(tag.get(), timestamp_ms - req_->info().media_origin_pos_ms_);

      // If pausing, don't read tags
      if ( pause_count_ > 0 ) {
        // If a seek operation is pending then stop and do the seek
        if ( seek_pos_ms_ >= 0 ) {
          break;
        }
        // .. else return, as we're paused
        in_data_processing_ = false;
        return;
      }
    }

    if ( is_eof_ && seek_offset_ < 0 ) {
      ILOG_INFO << "Stopping element on is_eof_";
      StopElement(false);
      in_data_processing_ = false;
      return;
    }
    in_data_processing_ = false;

    InitiateAioRequest(false);
  }
  void ProcessDataResume() {
    if ( !in_io_operation_ ) {
      ProcessData();
    }
  }

  void SendTag(const Tag* tag, int64 timestamp_ms) {
    DCHECK(!bootstrapping_);

    in_tag_processing_ = true;
    callback_->Run(tag, timestamp_ms);
    in_tag_processing_ = false;
  }
  void EndBootstrapping(int64 timestamp_ms, bool send_seek) {
    DCHECK(bootstrapping_);
    bootstrapping_ = false;

    // Always send SourceStartedTag
    SendTag(scoped_ref<Tag>(
        new SourceStartedTag(0,
            kDefaultFlavourMask,
            strutil::JoinPaths(element_name_, filename_relative_to_element_),
            strutil::JoinPaths(element_name_, filename_relative_to_element_),
            false,
            0)).get(),
            0);

    in_tag_processing_ = true;
    // Send the bootstrap
    bootstrapper_.PlayAtBegin(
      callback_, 0, kDefaultFlavourMask);
    in_tag_processing_ = false;

    // Send SeekPerformedTag if requested
    if ( send_seek ) {
      SendTag(scoped_ref<Tag>(
          new SeekPerformedTag(0, kDefaultFlavourMask)).get(),
            timestamp_ms - req_->info().media_origin_pos_ms_);
    }
  }

  void StopElement(bool forced) {
    ILOG_INFO << "Stop, sending EOS. File splitter stats: "
              << splitter_->stats().ToString();
    if ( !bootstrapping_ ) {
      SendTag(scoped_ref<Tag>(new SourceEndedTag(
          0, kDefaultFlavourMask,
          strutil::JoinPaths(element_name_, filename_relative_to_element_),
          strutil::JoinPaths(element_name_, filename_relative_to_element_))).
          get(), 0);
    } else {
      bootstrapping_ = false;
    }

    req_->set_controller(NULL);
    SendTag(scoped_ref<Tag>(new EosTag(
        0, kDefaultFlavourMask, forced)).get(), 0);
  }
  void ClearFileOperation() {
    if ( buffer_ != NULL ) {
      buffer_->EndUsage(full_callback_);
      aio_req_->buffer_ = NULL;
      buffer_ = NULL;
    }
    req_->set_controller(NULL);
    ::close(aio_req_->fd_);
    notify_closed_->Run(this);
    delete this;
  }
 private:
  net::Selector* selector_;
  streaming::Request* req_;
  const string filename_;
  const string filename_relative_to_element_;
  const string filekey_;
  const string element_name_;

  const bool disable_pause_;
  const bool disable_seek_;
  const bool disable_duration_;

  const int64 file_size_;

  io::AioManager* const aio_manager_;
  io::BufferManager* const buf_manager_;

  io::BufferManager::Buffer* buffer_;
  io::AioManager::Request* const aio_req_;
  TagSplitter* const splitter_;
  io::MemoryStream buf_;

  // current file read position
  int64 pos_;
  // skip these many bytes on read completion (used on seek between cuepoints)
  int64 read_skip_;
  // pending seek position and offset
  int64 seek_pos_ms_;
  int64 seek_offset_;

  bool bootstrapping_;
  Bootstrapper bootstrapper_;

  scoped_ref<CuePointTag> cue_points_;

  // send tag downstream
  streaming::ProcessingCallback* callback_;

  // the parent marks this flag when the FRS is in read pending mode.
  // On AioOperationCompleted, if is_orphaned_==true, the FRS deletes itself.
  bool is_orphaned_;

  // AIO Read pending
  bool in_io_operation_;
  // Data is being processed
  bool in_data_processing_;
  // Downstream callback is running
  bool in_tag_processing_;

  // Incremented by Pause(true), Decremented by Pause(false)
  int  pause_count_;
  // EOF reached
  bool is_eof_;
  // the parent marks this flag when the FRS is in read pending mode.
  // On AioOperationCompleted, if should_close_==true, the FRS sends EOS.
  bool should_close_;

  // calls ProcessData, which resumes streaming
  util::Alarm unpause_alarm_;
  // calls ProcessAioOperation, 2 use cases:
  //   - on aio operation completed
  //   - on buffer (BufferManager) ready
  Closure* full_callback_;
  // run this closure before deleting the frs. Notifies the parent
  // AioFileElement that this FRS is completely closed.
  Callback1<AioFileReadingStruct*>* notify_closed_;
};

const char AioFileElement::kElementClassName[] = "aio_file";

AioFileElement::AioFileElement(
    const char* _name,
    const char* id,
    ElementMapper* mapper,
    net::Selector* selector,
    SplitterCreator* splitter_creator,
    map<string, io::AioManager*>* aio_managers,
    io::BufferManager* buf_manager,
    Tag::Type tag_type,
    const char* home_dir,
    const char* file_pattern,
    const char* default_index_file,
    const char* data_key_prefix,
    bool disable_pause,
    bool disable_seek,
    bool disable_duration)
    : Element(kElementClassName, _name, id, mapper),
      caps_(tag_type, streaming::kDefaultFlavourMask),
      selector_(selector),
      splitter_creator_(splitter_creator),
      aio_managers_(aio_managers),
      home_dir_(strutil::NormalizePath(home_dir)),
      file_pattern_(file_pattern),
      default_index_file_(default_index_file),
      data_key_prefix_(data_key_prefix),
      disable_pause_(disable_pause),
      disable_seek_(disable_seek),
      disable_duration_(disable_duration),
      call_on_close_(NULL),
      buf_manager_(buf_manager),
      media_info_cache_(util::CacheBase::LRU, 100, 1000LL * 3600,
          &util::DefaultValueDestructor, NULL),
      notify_frs_closed_callback_(NewPermanentCallback(this,
          &AioFileElement::NotifyFrsClosed)) {
  ILOG_DEBUG << "AioFileElement created under home directory: " << home_dir;
}

////////////////////////////////////////////////////////////////////////////////

AioFileElement::~AioFileElement() {
  CHECK(file_ops_.empty());
  delete splitter_creator_;
  delete notify_frs_closed_callback_;
}

bool AioFileElement::Initialize() {
  // Our chance to scream
  if ( file_pattern_.HasError() ) {
    ILOG_ERROR << "file pattern [" << file_pattern_.regex()
              << "] has errors: " << file_pattern_.ErrorName();
    return false;
  }
  if ( !io::IsDir(home_dir_.c_str()) ) {
    ILOG_ERROR << "invalid home dir: [" << home_dir_ << "]";
    return false;
  }
  return true;
}

bool AioFileElement::HasMedia(const char* media, Capabilities* out) {
  const string filename = FileNameFromMedia(media);
  if ( !io::IsReadableFile(filename) ) {
    if ( !default_index_file_.empty() && io::IsDir(filename) &&
         io::IsReadableFile(
             strutil::JoinPaths(filename, default_index_file_)) ) {
      *out = caps_;
      return true;
    }
    return false;
  }
  *out = caps_;
  return true;
}

void AioFileElement::ListMedia(const char* media_dir,
                               ElementDescriptions* media) {
  const string dir_name = FileNameFromMedia(media_dir);
  vector<string> tmp_elements;
  io::RecursiveListing(dir_name, &tmp_elements, &file_pattern_);
  int len = dir_name.length();
  for ( vector<string>::const_iterator it = tmp_elements.begin();
        it != tmp_elements.end(); ++it ) {
    media->push_back(make_pair(
                         strutil::NormalizePath(name_ + "/" + it->substr(len)),
                         caps_));
  }
}
// declared in base/media_info_util.h
namespace util {
bool ExtractMediaInfoFromFile(const string& filename, MediaInfo* out);
}
bool AioFileElement::DescribeMedia(const string& media,
                                   MediaInfoCallback* callback) {
  const string filename = FileNameFromMedia(
      strutil::JoinPaths(name_, media).c_str());
  if ( filename == "" ) {
    ILOG_ERROR << "cannot find file: [" << filename << "], media: ["
               << media << "], looking in home_dir: [" << home_dir_ << "]"
                  ", pattern: " << file_pattern_.regex();
    return false;
  }

  // first: search cache
  const MediaInfo* media_info = media_info_cache_.Get(filename);
  ILOG_DEBUG << "AioFileElement::DescribeMedia cache "
            << (media_info == NULL ? "MISS" : "HIT") << " on media: " << media;
  if ( media_info == NULL ) {
    // second: read file and assemble a new MediaInfo
    MediaInfo* info = new MediaInfo();
    ILOG_ERROR << "AioFileElement::DescribeMedia is BLOCKING!"
                  " Implement an asynchronous method!";
    if ( !util::ExtractMediaInfoFromFile(filename, info) ) {
      delete media_info;
      return false;
    }
    // the cache is responsible with deleting the 'media_info'
    media_info_cache_.Add(filename, info);
    media_info = info;
  }
  callback->Run(media_info);
  return true;
}

bool AioFileElement::AddRequest(const char* media_name,
                                streaming::Request* req,
                                streaming::ProcessingCallback* callback) {
  CHECK(file_ops_.find(req) == file_ops_.end());
  if ( call_on_close_ != NULL ) {
    ILOG_ERROR << "AddRequest failed, AioFileElement is closing";
    return false;
  }
  if ( !caps_.IsCompatible(req->caps()) ) {
    ILOG_ERROR << "AddRequest failed: mismatching caps."
               << caps_.ToString() << " vs. " << req->caps().ToString();
    return false;
  }

  string opened_file;
  string filename_relative_to_element = FileNameFromMedia(media_name, false);
  string filename = FileNameFromMedia(media_name, true);
  if ( filename.empty() ) {
    ILOG_ERROR << "cannot find file: [" << filename << "], media_name: ["
               << media_name << "], looking in home_dir: [" << home_dir_ << "]"
                  ", pattern: " << file_pattern_.regex();
    return false;
  }
#ifdef O_DIRECT
  int fd = ::open(filename.c_str(), O_RDONLY | O_DIRECT);
#else
  int fd = ::open(filename.c_str(), O_RDONLY);
#endif
  if ( fd < 0 ) {
    ILOG_ERROR << "cannot open: [" << filename << "]"
                  ", error: " << GetLastSystemErrorDescription();
    if ( default_index_file_.empty() || !io::IsDir(filename) ) {
      return false;
    }
    const string index_filename(strutil::JoinPaths(
                                      filename, default_index_file_));
#ifdef O_DIRECT
    fd = ::open(index_filename.c_str(), O_RDONLY | O_DIRECT);
#else
    fd = ::open(index_filename.c_str(), O_RDONLY);
#endif
    if ( fd < 0 ) {
      ILOG_ERROR << "cannot open: [" << index_filename << "]"
                    ", error: " << GetLastSystemErrorDescription();
      return false;
    }
    opened_file = index_filename;
  } else {
    opened_file = filename;
  }
  string path(filename);
  io::AioManager* const aio_manager = io::FindPathBased(aio_managers_, path);
  if ( aio_manager == NULL ) {
    ::close(fd);
    ILOG_ERROR << "Cannot find an AIO manager for file: " << filename;
    return false;
  }

  struct stat fs;
  if ( ::fstat(fd, &fs) != 0 ) {
    ::close(fd);
    ILOG_WARNING << "Cannot stat: " << opened_file;
    return false;
  }

#ifdef O_DIRECT
  //  Important note - lio_listio may fail on some kernels for small files
  //  opened with O_DIRECT.
  //
  //  Looks like re-opening w/o O_DIRECT is ok.
  //
  if ( fs.st_size < buf_manager_->alignment() ) {
    ::close(fd);
    fd = ::open(opened_file.c_str(), O_RDONLY);
    if ( fd < 0 ) {
      ILOG_WARNING << "Cannot re-open small file: [" << opened_file << "]";
      return false;
    }
  }
#endif

  ILOG_INFO << "Creating an AIO file reader: " << filename
            << " With manager: " << aio_manager->name()
            << " -- Origin: " << req->info().media_origin_pos_ms_
            << " -- Limit: " << req->info().limit_ms_;

  req->mutable_serving_info()->size_ = fs.st_size;
  if ( req->info().seek_pos_ms_ <= 0 ) {
    req->mutable_serving_info()->offset_ = 0;
  }
  req->mutable_caps()->IntersectCaps(caps_);
  const string filekey = data_key_prefix_.empty()
                         ? media_name
                         : data_key_prefix_ + filename.substr(home_dir_.size());

  AioFileReadingStruct* const frs = new AioFileReadingStruct(
      selector_,
      req,
      filename,
      filename_relative_to_element,
      filekey,
      name(),
      fs.st_size,
      disable_pause_,
      disable_seek_,
      disable_duration_,
      aio_manager,
      buf_manager_,
      fd,
      splitter_creator_->CreateSplitter(media_name, caps_.tag_type_),
      callback,
      notify_frs_closed_callback_);
  if ( !frs->InitiateAioRequest(true) ) {
    ILOG_ERROR << "InitiateAioRequest failed";
    delete frs;
    return false;
  }

  file_ops_.insert(make_pair(req, frs));
  return true;
}

void AioFileElement::RemoveRequest(streaming::Request* req) {
  FileOpMap::iterator it = file_ops_.find(req);
  if ( it == file_ops_.end() ) {
    ILOG_FATAL << "RemoveRequest cannot find req: " << req->ToString();
    return;
  }
  AioFileReadingStruct* frs = it->second;
  // Mark the FRS orphaned, or tell him to destroy itself now.
  // The FRS may wait crt aio op completion, then delete itself, then call
  // NotifyFrsDeleted(): that's were the frs is removed from file_ops_.
  if ( frs->in_io_operation() || frs->in_tag_processing() ) {
    frs->set_is_orphaned(true);
  } else {
    frs->ClearFileOperation();
  }
}

void AioFileElement::Close(Closure* call_on_close) {
  CHECK_NULL(call_on_close_) << "Duplicate Close()";
  call_on_close_ = call_on_close;

  selector_->RunInSelectLoop(NewCallback(this,
      &AioFileElement::InternalClose));
}

void AioFileElement::InternalClose() {
  if (call_on_close_ == NULL) {
    DCHECK(file_ops_.empty());
    return;
  }

  if ( file_ops_.empty() ) {
    ILOG_DEBUG << "No ops, close completed";
    selector_->RunInSelectLoop(call_on_close_);
    call_on_close_ = NULL;
    return;
  }

  // need to copy all operations to a temp, as EOS-es trigger RemoveRequest
  // which in turn create iterator trouble
  vector<AioFileReadingStruct*> tmp;
  transform(file_ops_.begin(), file_ops_.end(), back_inserter(tmp),
            __gnu_cxx::select2nd<FileOpMap::value_type>());
  for ( uint32 i = 0; i < tmp.size(); i++ ) {
    AioFileReadingStruct* s = tmp[i];
    s->set_should_close(true);
    if ( !s->in_io_operation() && !s->in_tag_processing() ) {
      s->StopElement(true);
    }
  }
}

void AioFileElement::NotifyFrsClosed(AioFileReadingStruct* frs) {
  size_t count = file_ops_.erase(frs->req());
  CHECK(count == 1) << "Cannot find frs: " << frs->name() << " on request: "
                    << frs->req()->ToString();
  ILOG_DEBUG << "FRS closed, " << file_ops_.size() << " remaining";
  // If there are no more active FRS and Close() was requested => complete close
  if ( file_ops_.empty() && call_on_close_ != NULL ) {
    ILOG_DEBUG << "No more active FRS, completing close..";
    selector_->RunInSelectLoop(call_on_close_);
    call_on_close_ = NULL;
  }
}

string AioFileElement::FileNameFromMedia(const char* media, bool full) {
  if ( *media == 0 ) {
    return "";
  }
  if ( *media == '/' ) {
    media++;
  }
  if ( name_ == media ) {
    return home_dir_;
  }
  pair<string, string> p = strutil::SplitFirst(media, "/");
  if ( name_ != p.first ) {
    ILOG_ERROR << "Bad media, not our name: [" << p.first << "]";
    return "";
  }
  if ( !file_pattern_.Matches(p.second) ) {
    ILOG_ERROR << "Bad media, mismatching pattern: " << file_pattern_.regex()
               << " for path: [" << p.second << "]";
    return "";
  }
  if ( full ) {
    return  strutil::JoinPaths(home_dir_, p.second);
  }
  return p.second;
}

}
