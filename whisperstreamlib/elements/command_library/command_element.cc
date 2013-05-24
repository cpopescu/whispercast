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

// @@@ NEEDS REFACTORING - CLOSE/TAG DISTRIBUTION

#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include <whisperlib/common/base/errno.h>
#include <whisperlib/net/base/selectable_filereader.h>
#include <whisperstreamlib/base/tag_distributor.h>
#include <whisperstreamlib/elements/command_library/command_element.h>

namespace streaming {

const char CommandElement::kElementClassName[] = "command";

////////////////////////////////////////////////////////////////////////////////
//
// Helper class:
//    - CommandElementData
//
class CommandElementData {
 public:
  typedef Callback1<CommandElementData*> CloseCallback;
  CommandElementData(net::Selector* selector,
                     CloseCallback* close_callback,
                     const string& name,
                     MediaFormat media_format,
                     const string& command,
                     bool should_reopen)
    : selector_(selector),
      close_callback_(close_callback),
      name_(name),
      media_format_(media_format),
      command_(command),
      should_reopen_(should_reopen),
      splitter_(NULL),
      distributor_(streaming::kDefaultFlavourMask, name),
      reader_(NULL),
      stream_ts_(0),
      pid_(-1),
      fd_0_(-1) {
  }
  virtual ~CommandElementData() {
    CHECK_EQ(fd_0_, INVALID_FD_VALUE);
    CHECK_EQ(pid_, -1);
    delete splitter_;
    splitter_ = NULL;
    CHECK_NULL(reader_);
  }

  const TagSplitter* splitter() const {
    return splitter_;
  }

  // Call this after creating a CommandElementData to start the actual process
  void Initialize() {
    int fd[2];
    CHECK(::pipe(fd) == 0) << "Error creating a pipe: "
                           << GetLastSystemErrorDescription();
    pid_ = ::fork();
    CHECK(pid_ != -1) << "Error forking: "
                      << GetLastSystemErrorDescription();
    if ( pid_ == 0 ) {
      setsid();  // set us as a process group leader
                 // (kill us -> kill all under us)
      // In child -> redirect stdout to fd[1]
      ::close(fd[0]);
      ::close(1);
      CHECK_EQ(1, dup(fd[1]));
      ::close(fd[1]);
      execl(command_.c_str(), "", NULL);
      LOG_INFO << " =========> Command pid ended: " << command_;
      sleep(1);
      exit(1);
    } else {
      ::close(fd[1]);
      fd_0_ = fd[0];
      // In main - register a pipe reader
      LOG_INFO << " Starting element: " << name_ << " w/ command: ["
               << command_ << "] - pid: " << pid_;
      if ( splitter_ == NULL ) {
        // TODO(cpopescu): create this w/ a function
        splitter_ = CreateSplitter(name_, media_format_);
      }
      CHECK(reader_ == NULL);
      reader_ = new net::SelectableFilereader(selector_);

      CHECK(reader_->InitializeFd(
              fd[0],
              NewPermanentCallback(this, &CommandElementData::ProcessElementData),
              true,
              NewCallback(this, &CommandElementData::ClosedElement)));
    }
  }

  const string& name() const {
    return name_;
  }

  bool AddRequest(Request* req, ProcessingCallback* callback) {
    distributor_.add_callback(req, callback);
    return true;
  }
  void RemoveRequest(Request* req) {
    distributor_.remove_callback(req);
    if ( distributor_.count() == 0 && pid_ == -1 ) {
      close_callback_->Run(this);
    }
  }

 private:
  // Processes some more data from the pipe
  void ProcessElementData(io::MemoryStream* io) {
    CHECK_NOT_NULL(splitter_);
    CHECK(pid_ != -1);
    while ( true ) {
      int64 timestamp_ms;
      scoped_ref<Tag> tag;
      TagReadStatus status = splitter_->GetNextTag(
          io, &tag, &timestamp_ms, true);
      if ( status == streaming::READ_CORRUPTED ||
           status == streaming::READ_EOF ||
           status == streaming::READ_CORRUPTED ) {
        LOG_ERROR << "Error on element: " << name_
                  << ", status: " << TagReadStatusName(status)
                  << ", cmd: [" << command_ << "]"
                  << " - KILLING - (for a restart) - pid: " << pid_;
        Close();
        return;
      }
      if ( status != streaming::READ_OK ) {
        break;
      }
      stream_ts_ = timestamp_ms;
      distributor_.DistributeTag(tag.get(), timestamp_ms);
    };
  }
  // Callback when pipe is closed (i.e. command ended)
  void ClosedElement() {
    LOG_INFO << name_ << " cmd[" << command_ << "] - Closed element !";
    if ( fd_0_ != INVALID_FD_VALUE ) {
      ::close(fd_0_);
      fd_0_ = INVALID_FD_VALUE;
    }
    selector_->DeleteInSelectLoop(reader_);
    reader_ = NULL;

    if ( !selector_->IsExiting() ) {
      if ( should_reopen_ ) {
        LOG_INFO << " Reopening element : " << name_ << " cmd:["
                 << command_ << "]";
        Initialize();
        return;
      }
    }

    LOG_INFO << " Element : " << name_ << " cmd:[" << command_ << "]"
             << " - closed. ";
    Close();
  }

 public:
  // Close everything inside this command element
  void Close() {
    if ( fd_0_ != -1 ) {
      ::close(fd_0_);
      fd_0_ = INVALID_FD_VALUE;
    }
    if ( reader_ != NULL ) {
      reader_->Close();
      delete reader_;
    }
    if ( pid_ > 0 ) {
      LOG_INFO << " ===========>  KILLING on delete: " << pid_;
      kill(-pid_, SIGKILL);  // kill by group ..
      pid_ = -1;
    }
    if ( distributor_.count() == 0 ) {
      close_callback_->Run(this);
      return;
    }
    distributor_.DistributeTag(scoped_ref<Tag>(new SourceEndedTag(
          0, kDefaultFlavourMask, name_, name_)).get(), 0);
    distributor_.CloseAllCallbacks(true);
  }


 private:
  // Main network selector
  net::Selector* const selector_;
  // We call this when we are totally done
  CloseCallback* close_callback_;
  // name of the element
  const string name_;
  const string path_;
  // Media Format to expect
  MediaFormat media_format_;
  // command to execute for the element
  const string command_;
  // should the command be reopened in case of error ?
  const bool should_reopen_;

  // Closure to call when eveything is done..

  // this processes the input to obtain tags
  streaming::TagSplitter* splitter_;
  // Manages the callbacks for us
  TagDistributor distributor_;
  // reads data from the pipe
  net::SelectableFilereader* reader_;
  // stream time = last tag time
  int64 stream_ts_;
  // the process that executes the command.
  int pid_;
  // our side of the pipe
  int fd_0_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(CommandElementData);
};

////////////////////////////////////////////////////////////////////////////////
//
// CommandElement implementation
//
CommandElement::CommandElement(
  const string& name,
  ElementMapper* mapper,
  net::Selector* selector)
    : Element(kElementClassName, name, mapper),
    selector_(selector),
    close_completed_(NULL) {
}

CommandElement::~CommandElement() {
  for ( ElementMap::const_iterator it = elements_.begin();
        it != elements_.end(); ++it ) {
    delete it->second;
  }
}

CommandElementData* CommandElement::GetElement(const string& name) {
  ElementMap::const_iterator it = elements_.find(string(name));
  if ( it == elements_.end() ) {
    LOG_INFO << " Command finding element for: " << name << " - none";
    return NULL;
  }
  return it->second;
}

void CommandElement::ClosedElementNotification(CommandElementData* data) {
  elements_.erase(data->name());
  selector_->DeleteInSelectLoop(data);

  if ( close_completed_ != NULL && elements_.empty() ) {
    selector_->RunInSelectLoop(close_completed_);
    close_completed_ = NULL;
  }
}

bool CommandElement::AddElement(const string& ename,
                                MediaFormat media_format,
                                const string& command,
                                bool should_reopen) {
  const string full_name = strutil::JoinPaths(name(), ename);
  ElementMap::const_iterator it = elements_.find(string(full_name));
  if ( it != elements_.end() ) {
    return false;
  }
  CommandElementData* cmd = new CommandElementData(
    selector_,
    NewCallback(this, &CommandElement::ClosedElementNotification),
    full_name, media_format, command, should_reopen);
  elements_.insert(make_pair(full_name, cmd));
  cmd->Initialize();
  return true;
}

bool CommandElement::AddRequest(const string& media, Request* req,
                                ProcessingCallback* callback) {
  CommandElementData* const src = GetElement(media);
  if ( src == NULL ) {
    LOG_ERROR << "Cannot find element, media: [" << media << "]";
    return false;
  }
  if ( !src->AddRequest(req, callback) ) {
    LOG_ERROR << "Cannot AddRequest";
    return false;
  }
  requests_.insert(make_pair(req, src));
  return true;
}

void CommandElement::RemoveRequest(streaming::Request* req) {
  RequestMap::iterator it = requests_.find(req);
  CHECK(it != requests_.end()) << "Cannot find req: " << req->ToString();
  it->second->RemoveRequest(req);
}

bool CommandElement::HasMedia(const string& media) {
  return GetElement(media) != NULL;
}

void CommandElement::ListMedia(const string& media, vector<string>* out) {
  for ( ElementMap::const_iterator it = elements_.begin();
        it != elements_.end(); ++it ) {
    out->push_back(it->first);
  }
}
bool CommandElement::DescribeMedia(const string& media,
    MediaInfoCallback* callback) {
  ElementMap::const_iterator it = elements_.find(media);
  if ( it == elements_.end() ||
       it->second->splitter() == NULL ) {
    return false;
  }
  callback->Run(&(it->second->splitter()->media_info()));
  return true;
}

void CommandElement::Close(Closure* call_on_close) {
  CHECK_NULL(close_completed_);
  if ( elements_.empty() ) {
    selector_->RunInSelectLoop(call_on_close);
    return;
  }
  close_completed_ = call_on_close;
  for ( ElementMap::iterator it = elements_.begin();
        it != elements_.end(); ++it ) {
    it->second->Close();
  }
}
}
