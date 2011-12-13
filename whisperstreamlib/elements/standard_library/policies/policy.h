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
#ifndef __STREAMING_ELEMENTS_POLICY_H__
#define __STREAMING_ELEMENTS_POLICY_H__

#include <string>
#include <vector>
#include <deque>
#include <utility>
#include <algorithm>

#include <whisperlib/common/base/date.h>
#include <whisperlib/common/io/checkpoint/state_keeper.h>
#include <whisperlib/net/base/selector.h>
#include <whisperstreamlib/base/element.h>
#include <whisperlib/net/rpc/lib/server/rpc_http_server.h>

#include <whisperstreamlib/elements/standard_library/auto/standard_library_invokers.h>

namespace streaming {

//////////////////////////////////////////////////////////////////////

// Simple policy - picks a random next element
class RandomPolicy : public Policy {
 public:
  static const char kPolicyClassName[];
  RandomPolicy(const char* name,
               PolicyDrivenElement* element,
               bool is_temp_policy,
               io::StateKeepUser* global_state_keeper,
               io::StateKeepUser* local_state_keeper,
               int max_history_size)
      : Policy(kPolicyClassName, name, element),
        is_temp_policy_(is_temp_policy),
        global_state_keeper_(global_state_keeper),
        local_state_keeper_(local_state_keeper),
        max_history_size_(max_history_size) {
  }
  virtual ~RandomPolicy() {
    if ( is_temp_policy_ ) {
      ClearState();
    }
    delete global_state_keeper_;
    delete local_state_keeper_;
  }
  virtual bool Initialize() {
    LoadState();
    SaveState();
    return GoToNext();
  }
  virtual void Reset() {
    next_to_play_.clear();
    history_.clear();
    SaveState();
  }
  virtual bool NotifyTag(const streaming::Tag*, int64) {
    return true;
  }
  virtual string GetPolicyConfig() { return ""; }

  bool LoadState();
  bool SaveState();
  void ClearState();

  bool NotifyEos() {
    return GoToNext();
  }
  bool GoToNext() {
    if ( available_.empty() ) {
      streaming::ElementDescriptions media;
      element_->ListElementMedia("", &media);
      for ( int i = 0; i < media.size(); ++i ) {
        available_.push_back(media[i].first);
      }
      if ( available_.empty() ) {
        return false;
      }
    }

    if ( next_to_play_.empty() ) {
      crt_ = available_[random() % available_.size()];
    } else {
      crt_ = next_to_play_.front();
      next_to_play_.pop_front();
    }
    if ( !element_->SwitchCurrentMedia(crt_, NULL, true) ) {
      crt_.clear();
      return false;
    }
    history_.push_back(crt_);
    if ( history_.size() > max_history_size_ ) {
      history_.pop_front();
    }
    SaveState();
    return true;
  }

  bool GoToPrev() {
    if ( history_.empty() ) {
      return false;
    }
    crt_ = history_.back();
    const bool success = element_->SwitchCurrentMedia(crt_, NULL, true);
    history_.pop_back();
    SaveState();
    return success;
  }

  bool AddToPlay(const string& media_name) {
    streaming::Capabilities caps;
    if ( !element_->HasElementMedia(media_name.c_str(), &caps) ) {
      return false;
    }
    next_to_play_.push_back(media_name);
    SaveState();
    return true;
  }

 protected:
  const bool is_temp_policy_;
  vector<string> available_;
  io::StateKeepUser* const global_state_keeper_;
  io::StateKeepUser* const local_state_keeper_;
  deque<string> next_to_play_;         // what to play next - if any
 private:
  const int max_history_size_;
  deque<string> history_;              // what we played so far
  string crt_;

  DISALLOW_EVIL_CONSTRUCTORS(RandomPolicy);
};

//////////////////////////////////////////////////////////////////////

// A policy that picks from a list

class PlaylistPolicy : public Policy,
                       public ServiceInvokerPlaylistPolicyService {
 public:
  static const char kPolicyClassName[];
  PlaylistPolicy(const char* name,
                 PolicyDrivenElement* element,
                 bool is_temp_policy,
                 io::StateKeepUser* global_state_keeper,
                 io::StateKeepUser* local_state_keeper,
                 const vector<string>& playlist,
                 bool loop_playlist,
                 const char* rpc_path,
                 const char* local_rpc_path,
                 rpc::HttpServer* rpc_server)
      : Policy(kPolicyClassName, name, element),
        ServiceInvokerPlaylistPolicyService(
            ServiceInvokerPlaylistPolicyService::GetClassName()),
        is_temp_policy_(is_temp_policy),
        global_state_keeper_(global_state_keeper),
        local_state_keeper_(local_state_keeper),
        playlist_(playlist.size()),
        loop_playlist_(loop_playlist),
        crt_(-1),
        next_to_play_(-1),
        next_next_to_play_(-1),
        rpc_path_(rpc_path),
        local_rpc_path_(local_rpc_path),
        rpc_server_(rpc_server),
        is_registered_(false) {
    copy(playlist.begin(), playlist.end(), playlist_.begin());
  }
  virtual ~PlaylistPolicy() {
    if ( is_registered_ ) {
      rpc_server_->UnregisterService(local_rpc_path_, this);
    }
    if ( is_temp_policy_ ) {
      ClearState();
    }
    delete global_state_keeper_;
    delete local_state_keeper_;
  }

  virtual bool Initialize() {
    if ( rpc_server_ != NULL ) {
      is_registered_ = rpc_server_->RegisterService(local_rpc_path_, this);
      if ( is_registered_ && local_rpc_path_ != rpc_path_ ) {
        rpc_server_->RegisterServiceMirror(rpc_path_, local_rpc_path_);
      }
    }
    LoadState();
    SaveState();
    return GoToNext() && (is_registered_ || rpc_server_ == NULL);
  }


  virtual void Reset() {
    crt_ = -1;
    next_to_play_ = -1;
    next_next_to_play_ = -1;
    SaveState();
  }

  virtual bool NotifyTag(const streaming::Tag*, int64) {
    return true;
  }
  virtual string GetPolicyConfig() { return ""; }

  bool LoadState();
  bool SaveState();
  void ClearState();

  bool NotifyEos() {
    return GoToNext();
  }
  bool GoToNext() {
    if ( playlist_.empty() ) {
      LOG_ERROR << " Playlist policy for " << element_->name()
                << " has empty playlist";
      return false;
    }
    if ( next_to_play_ >= 0 && next_to_play_ < playlist_.size() ) {
      crt_ = next_to_play_;
      if ( next_next_to_play_ >= 0 && next_next_to_play_ < playlist_.size() ) {
        next_to_play_ = next_next_to_play_;
        next_next_to_play_ = -1;
      } else {
        next_to_play_ = -1;
      }
    } else if ( ++crt_ >= playlist_.size() ) {
      if ( !loop_playlist_ ) {
        LOG_INFO << "End of playlist for " << element_->name();
        SaveState();
        return false;
      }
      crt_ = 0;
    }
    SaveState();
    const bool ret = element_->SwitchCurrentMedia(playlist_[crt_],
                                                  NULL,
                                                  true);
    if ( !ret ) {
      LOG_WARNING << " Cannot switch for: " << element_->name()
                  << " to: " << playlist_[crt_] << " ending playlist";
    }
    return ret;
  }

  bool GoToPrev() {
    if ( playlist_.empty() ) {
      LOG_ERROR << " Playlist policy for " << element_->name()
                << " has empty playlist";
      return false;
    }
    if ( --crt_ < 0 ) {
      if ( !loop_playlist_ ) {
        SaveState();
        return false;
      }
      crt_ = playlist_.size() - 1;
    }
    SaveState();
    return element_->SwitchCurrentMedia(playlist_[crt_], NULL, true);
  }
  bool AddToPlay(const string& name) {
    for ( int i = 0; i < playlist_.size(); ++i ) {
      if ( name == playlist_[i] ) {
        next_to_play_ = i;
        if ( local_state_keeper_ != NULL ) {
          local_state_keeper_->SetValue("next_next_to_play",
              strutil::StringPrintf("%d", next_to_play_));
        }
        return true;
      }
    }
    return false;
  }
  const vector<string>& playlist() const {
    return playlist_;
  }
  bool loop_playlist() const {
    return loop_playlist_;
  }
 protected:
  virtual void SetPlaylist(rpc::CallContext< MediaOperationErrorData >* call,
                           const SetPlaylistPolicySpec& playlist);
  virtual void GetPlaylist(rpc::CallContext< PlaylistPolicySpec >* call);
  virtual void GetPlayInfo(rpc::CallContext<PolicyPlayInfo>* call);

 private:
  void GetPlaylist(PlaylistPolicySpec* playlist) const;
  void SetPlaylist(const PlaylistPolicySpec& playlist);

  const bool is_temp_policy_;
  io::StateKeepUser* const global_state_keeper_;
  io::StateKeepUser* const local_state_keeper_;
  vector<string> playlist_;
  bool loop_playlist_;
  int crt_;
  int next_to_play_;
  int next_next_to_play_;  // used just to save state..
  const string rpc_path_;
  const string local_rpc_path_;
  rpc::HttpServer* const rpc_server_;
  bool is_registered_;

  DISALLOW_EVIL_CONSTRUCTORS(PlaylistPolicy);
};

//////////////////////////////////////////////////////////////////////

class TimedPlaylistPolicy : public Policy {
 public:
  static const char kPolicyClassName[];
  // What to do when we are empty (and time did not end..) ?
  enum EmptyPolicy {
    FIRST_EMPTY_POLICY = 0,
    POLICY_REPLAY = 0,   // we replay the playlist item until the time ends
    POLICY_NEXT,     // we go to the next in playlist
    POLICY_WAIT,     // we play nothing and wait for time completion
    NUM_EMPTY_POLICY,
  };
  TimedPlaylistPolicy(const char* name,
                      PolicyDrivenElement* element,
                      net::Selector* selector,
                      bool is_temp_policy,
                      io::StateKeepUser* global_state_keeper,
                      io::StateKeepUser* local_state_keeper,
                      const vector< pair<int64, string> >& playlist,
                      EmptyPolicy policy,
                      bool loop_playlist)
      : Policy(kPolicyClassName, name, element),
        selector_(selector),
        is_temp_policy_(is_temp_policy),
        global_state_keeper_(global_state_keeper),
        local_state_keeper_(local_state_keeper),
        playlist_(playlist.size()),
        policy_(policy),
        loop_playlist_(loop_playlist),
        crt_(-1),
        next_to_play_(-1),
        next_next_to_play_(-1),
        last_switch_time_(-1),
        alarm_callback_(NewPermanentCallback(
                            this, &TimedPlaylistPolicy::Next)) {
    copy(playlist.begin(), playlist.end(), playlist_.begin());
  }
  virtual ~TimedPlaylistPolicy() {
    if ( is_temp_policy_ ) {
      ClearState();
    }
    delete global_state_keeper_;
    delete local_state_keeper_;
    selector_->UnregisterAlarm(alarm_callback_);
    delete alarm_callback_;
  }
  virtual bool Initialize() {
    LoadState();
    SaveState();
    return GoToNext();
  }
  virtual void Reset() {
    crt_ = -1;
    next_to_play_ = -1;
    next_next_to_play_ = -1;
    last_switch_time_ = -1;
    SaveState();
  }
  virtual bool NotifyTag(const streaming::Tag*, int64) {
    return true;
  }
  virtual string GetPolicyConfig() { return ""; }


  bool LoadState();
  bool SaveState();
  void ClearState();

  bool NotifyEos() {
    switch ( policy_ ) {
      case POLICY_REPLAY:
        return PlayCurrent();
      case POLICY_NEXT:
        return GoToNext();
      case POLICY_WAIT:
      default:
        return true;
    }
    return false;
  }

  bool GoToNext() {
    if ( playlist_.empty() ) {
      LOG_ERROR << " Playlist policy for " << element_->name()
                << " has empty playlist";
      return false;
    }
    if ( next_to_play_ >= 0 &&  next_to_play_ < playlist_.size() ) {
      crt_ = next_to_play_;
      if ( next_next_to_play_ >= 0 && next_next_to_play_ < playlist_.size() ) {
        next_to_play_ = next_next_to_play_;
        next_next_to_play_ = -1;
      } else {
        next_to_play_ = -1;
      }
    } else if ( ++crt_ >= playlist_.size() ) {
      if ( !loop_playlist_ ) {
        SaveState();
        return false;
      }
      crt_ = 0;
    }
    return PlayCurrent();
  }

  bool GoToPrev() {
    if ( playlist_.empty() ) {
      LOG_ERROR << " Playlist policy for " << element_->name()
                << " has empty playlist";
      return false;
    }
    if ( --crt_ < 0 ) {
      if ( !loop_playlist_ ) {
        SaveState();
        return false;
      }
      crt_ = playlist_.size() - 1;
    }
    return PlayCurrent();
  }

  bool AddToPlay(const string& name) {
    for ( int i = 0; i < playlist_.size(); ++i ) {
      if ( name == playlist_[i].second ) {
        next_to_play_ = i;
        if ( local_state_keeper_ != NULL ) {
          local_state_keeper_->SetValue("next_next_to_play",
              strutil::StringPrintf("%d", next_to_play_));
        }
        return true;
      }
    }
    return false;
  }

 private:
  // Helper so we can make it a Callback*
  void Next() {
    GoToNext();
  }
  bool PlayCurrent() {
    bool success = element_->SwitchCurrentMedia(playlist_[crt_].second,
                                                NULL,
                                                true);
    if ( !success ) {
      // TODO(cpoepscu): is it fine to do this --
      //                         well, probably ..for now
      LOG_ERROR << "Invalid playlist element : " << playlist_[crt_].second;
    }
    const int64 last_switch_time = timer::Date::Now();
    if ( last_switch_time_ > 0 && last_switch_time > last_switch_time_ ) {
      const int64 delta = (playlist_[crt_].first -
                           (last_switch_time - last_switch_time_));
      selector_->RegisterAlarm(alarm_callback_,
                                 max(static_cast<int64>(0), delta));
    } else {
      selector_->RegisterAlarm(alarm_callback_,
                                 playlist_[crt_].first);
    }
    last_switch_time_ = last_switch_time;
    success = SaveState() && success;
    last_switch_time_ = 0;  // so  it will not count for the next switch

    return success;
  }

 private:
  net::Selector* selector_;
  const bool is_temp_policy_;
  io::StateKeepUser* const global_state_keeper_;
  io::StateKeepUser* const local_state_keeper_;
  vector< pair<int64, string> > playlist_;
  EmptyPolicy policy_;
  bool loop_playlist_;
  int crt_;
  int next_to_play_;
  int next_next_to_play_;  // used just to save state..
  int64 last_switch_time_;
  Closure* alarm_callback_;

  DISALLOW_EVIL_CONSTRUCTORS(TimedPlaylistPolicy);
};

//////////////////////////////////////////////////////////////////////

// A policy which accepts switch commands through RPC. The policy has just 1
// default media, but through command it can switch to any internal media.

class OnCommandPolicy : public Policy,
                        public ServiceInvokerSwitchPolicyService {
 public:
  static const char kPolicyClassName[];
  OnCommandPolicy(const char* name,
                  PolicyDrivenElement* element,
                  bool is_temp_policy,
                  io::StateKeepUser* global_state_keeper,
                  io::StateKeepUser* local_state_keeper,
                  const string& default_media_name,
                  const char* rpc_path,
                  const char* local_rpc_path,
                  rpc::HttpServer* rpc_server)
      : Policy(kPolicyClassName, name, element),
        ServiceInvokerSwitchPolicyService(
            ServiceInvokerSwitchPolicyService::GetClassName()),
        is_temp_policy_(is_temp_policy),
        global_state_keeper_(global_state_keeper),
        local_state_keeper_(local_state_keeper),
        default_media_name_(default_media_name),
        rpc_path_(rpc_path),
        local_rpc_path_(local_rpc_path),
        rpc_server_(rpc_server),
        is_registered_(false) {
  }

  virtual ~OnCommandPolicy() {
    if ( is_registered_ ) {
      rpc_server_->UnregisterService(local_rpc_path_, this);
    }
    if ( is_temp_policy_ ) {
      ClearState();
    }
    delete global_state_keeper_;
    delete local_state_keeper_;
  }

  ///////////////////////////////////////////////////////////////////////
  // Policy methods
  //
  virtual bool Initialize();
  virtual void Reset();
  virtual bool NotifyEos();
  virtual bool NotifyTag(const Tag* tag, int64 timestamp_ms);
  virtual string GetPolicyConfig();

  //////////////////////////////////////////////////////////////////////
  bool LoadState();
  bool SaveState();
  void ClearState();

  // commands the underlying element to switch to the given media
  bool PlayMedia(const string& media);

  //////////////////////////////////////////////////////////////////////
  // ServiceInvokerSwitchPolicyService methods
  //
  virtual void SwitchPolicy(rpc::CallContext< MediaOperationErrorData >* call,
                            const string& media_name,
                            const bool set_as_default,
                            const bool also_switch);
  virtual void GetDefaultMedia(rpc::CallContext< string >* call);
  virtual void GetPlayInfo(rpc::CallContext<PolicyPlayInfo>* call);

 private:
  const bool is_temp_policy_;
  io::StateKeepUser* const global_state_keeper_;
  io::StateKeepUser* const local_state_keeper_;

  // determined on creation or by SwitchPolicy() command
  string default_media_name_;
  // determined by the last SwitchPolicy() command also_switch=true
  string crt_media_name_;
  // determined by the last SwitchPolicy() command also_switch=false
  string next_media_name_;

  const string rpc_path_;
  const string local_rpc_path_;
  rpc::HttpServer* const rpc_server_;
  bool is_registered_;

  DISALLOW_EVIL_CONSTRUCTORS(OnCommandPolicy);
};
}
#endif  // __STREAMING_ELEMENTS_POLICY_H__
