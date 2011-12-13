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

// Utility to distribute tags to multiple ProcessingCallbacks. It includes a
// CallbackManager and a Bootstrapper. This functionality was part of
// the splitter.

#ifndef  __MEDIA_BASE_TAG_DISTRIBUTOR_H__
#define  __MEDIA_BASE_TAG_DISTRIBUTOR_H__

#include <whisperlib/common/base/types.h>
#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/request.h>
#include <whisperstreamlib/base/element.h>
#include <whisperstreamlib/base/callbacks_manager.h>
#include <whisperstreamlib/base/bootstrapper.h>

namespace streaming {

class TagDistributor {
 private:
  struct CallbackMapEntry {
    ProcessingCallback* callback_;
    bool done_;
  };
  typedef map<Request*, CallbackMapEntry> CallbackMap;

 public:
  // We take control of the controller pointer
  TagDistributor(uint32 flavour_mask,
      const string& name, bool bootstrap_media = false) :
      bootstrapper_(bootstrap_media),
      flavour_mask_(flavour_mask),
      name_(name),
      last_tag_ts_(0),
      distributing_tag_(false) {
    // sanity check: flavour_mask should contain only 1 flavour_id
    DCHECK(flavour_mask != 0 && (flavour_mask & (flavour_mask-1)) == 0)
        << "Illegal flavour_mask: " << flavour_mask
        << ", MUST contain just 1 flavour_id";
  }
  virtual ~TagDistributor() {
    DCHECK(running_.empty());
    DCHECK(to_bootstrap_.empty());
  }

  uint32 flavour_mask() const {
    return flavour_mask_;
  }

  size_t count() const {
    return running_.size() + to_bootstrap_.size();
  }

  size_t running_count() const {
    return running_.size();
  }
  size_t to_bootstrap_count() const {
    return to_bootstrap_.size();
  }

  int64 last_tag_ts() const {
    return last_tag_ts_;
  }

  void add_callback(Request* req, ProcessingCallback* callback) {
    DCHECK(!distributing_tag_);
    DCHECK(callback->is_permanent());

    DCHECK(running_.find(req) == running_.end())
        << "Double add_callback, callback: " << callback
        << ", req: " << req->ToString();
    DCHECK(to_bootstrap_.find(req) == to_bootstrap_.end())
        << "Double add_callback, callback: " << callback
        << ", req: " << req->ToString();

    CallbackMapEntry c;
    c.callback_ = callback;
    c.done_ = false;

    to_bootstrap_.insert(make_pair(req, c));
  }
  void remove_callback(Request* req) {
    DCHECK(!distributing_tag_);

    if ( running_.erase(req) == 0 ) {;
      CHECK(to_bootstrap_.erase(req) == 1);
    }
  }

  //////////////////////////////////////////////////////////////////////////

  // Runs PlayAtEnd() on the bootstrapper
  void Switch() {
    distributing_tag_ = true;
    for ( CallbackMap::iterator it = running_.begin();
          it != running_.end(); ++it ) {
      bootstrapper_.PlayAtEnd(
        it->second.callback_, last_tag_ts_, flavour_mask_);
    }
    distributing_tag_ = false;

    bootstrapper_.ClearBootstrap();
    last_tag_ts_ = 0;
  }

  // Sends a SOURCE_ENDED to the running callbacks and
  // moves them to the to_bootstrap
  void Reset() {
    DCHECK(!distributing_tag_);

    distributing_tag_ = true;

    for ( CallbackMap::iterator it = running_.begin();
          it != running_.end(); ++it ) {
      bootstrapper_.PlayAtEnd(
        it->second.callback_, last_tag_ts_, flavour_mask_);
    }

    if ( !name_.empty() ) {
      scoped_ref<Tag> source_ended_tag(new SourceEndedTag(0, flavour_mask_,
          name_, name_));

      DistributeTagInternal(
        running_, source_ended_tag.get(), last_tag_ts_);
    }

    distributing_tag_ = false;

    while ( !running_.empty() ) {
      to_bootstrap_[running_.begin()->first] = running_.begin()->second;
      running_.erase(running_.begin());
    }

    bootstrapper_.ClearBootstrap();
    last_tag_ts_ = 0;
  }

  // Sends a SOURCE_ENDED/EOS to the given callback
  void CloseCallback(Request* req, bool forced) {
    DCHECK(!distributing_tag_);

    scoped_ref<Tag> source_ended_tag_r(new SourceEndedTag(0, flavour_mask_,
        name_, name_));
    const Tag* source_ended_tag = name_.empty() ? NULL : source_ended_tag_r.get();

    scoped_ref<Tag> eos_tag(
      new EosTag(0, flavour_mask_, forced));

    distributing_tag_ = true;

    CallbackMap::iterator it = running_.find(req);
    if ( it != running_.end() ) {
      bootstrapper_.PlayAtEnd(
        it->second.callback_, last_tag_ts_, flavour_mask_);

      DCHECK(!it->second.done_);
      if ( source_ended_tag != NULL) {
        it->second.callback_->Run(source_ended_tag, 0);
      }
      it->second.callback_->Run(eos_tag.get(), last_tag_ts_);
      it->second.done_ = true;
    } else {
      it = to_bootstrap_.find(req);
      DCHECK(it != to_bootstrap_.end());

      DCHECK(!it->second.done_);
      it->second.callback_->Run(eos_tag.get(), last_tag_ts_);
      it->second.done_ = true;
    }
    distributing_tag_ = false;
  }
  // Sends a SOURCE_ENDED/EOS to the all the callbacks
  void CloseAllCallbacks(bool forced) {
    DCHECK(!distributing_tag_);

    scoped_ref<Tag> source_ended_tag_r(new SourceEndedTag(0, flavour_mask_,
        name_, name_));
    Tag* source_ended_tag = name_.empty() ? NULL : source_ended_tag_r.get();

    scoped_ref<Tag> eos_tag(
      new EosTag( 0, flavour_mask_, forced));

    distributing_tag_ = true;

    for ( CallbackMap::iterator it = running_.begin();
          it != running_.end(); ++it ) {
      bootstrapper_.PlayAtEnd(
        it->second.callback_, last_tag_ts_, flavour_mask_);

      DCHECK(!it->second.done_);
      if ( source_ended_tag != NULL) {
        it->second.callback_->Run(source_ended_tag, 0);
      }
      it->second.callback_->Run(eos_tag.get(), last_tag_ts_);
      it->second.done_ = true;
    }
    for ( CallbackMap::iterator it = to_bootstrap_.begin();
          it != to_bootstrap_.end(); ++it ) {
      DCHECK(!it->second.done_);
      it->second.callback_->Run(eos_tag.get(), last_tag_ts_);
      it->second.done_ = true;
    }

    distributing_tag_ = false;
  }

  // Distributes a tag to all our callbacks
  void DistributeTag(const Tag* tag, int64 timestamp_ms) {
    DCHECK(!distributing_tag_);

    // eat the bootstrap begin/bootstrap end tags as we're doing our own
    if ( tag->type() == streaming::Tag::TYPE_BOOTSTRAP_BEGIN ||
         tag->type() == streaming::Tag::TYPE_BOOTSTRAP_END ) {
      return;
    }

    distributing_tag_ = true;

    // bootstrap new callbacks. These callbacks are also in callbacks_manager_.
    while ( !to_bootstrap_.empty() ) {
      running_[to_bootstrap_.begin()->first] = to_bootstrap_.begin()->second;

      ProcessingCallback* callback = to_bootstrap_.begin()->second.callback_;
      to_bootstrap_.erase(to_bootstrap_.begin());

      // send SourceStartedTag
      if ( !name_.empty() ) {
        callback->Run(scoped_ref<Tag>(new SourceStartedTag(
            0, flavour_mask_,
            name_, name_)).get(), timestamp_ms);
      }

      bootstrapper_.PlayAtBegin(callback, timestamp_ms, flavour_mask_);
    }

    bootstrapper_.ProcessTag(tag, timestamp_ms);
    DistributeTagInternal(running_, tag, timestamp_ms);

    distributing_tag_ = false;
  }

private:
  // Distributes a tag to all our callbacks
  void DistributeTagInternal(
    CallbackMap& callbacks, const Tag* tag, int64 timestamp_ms) {
    for ( CallbackMap::const_iterator it = callbacks.begin();
          it != callbacks.end(); ++it ) {
      it->second.callback_->Run(tag, timestamp_ms);
    }
    last_tag_ts_ = timestamp_ms;
  }

 private:
  // keep important tags here, sent to each new client
  Bootstrapper bootstrapper_;

  // the things that are currently running..
  CallbackMap running_;
  // the things that we need to bootstrap..
  CallbackMap to_bootstrap_;

  uint32 flavour_mask_;

  string name_;

  int64 last_tag_ts_;

  // Bug trap: whenever we send tags downstream we set this flag.
  // No changes (add_callback, remove_callback..) must come in between.
  bool distributing_tag_;

  DISALLOW_EVIL_CONSTRUCTORS(TagDistributor);
};

} // namespace streaming

#endif // __MEDIA_BASE_TAG_DISTRIBUTOR_H__
