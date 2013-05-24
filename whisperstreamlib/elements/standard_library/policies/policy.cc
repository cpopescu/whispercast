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

#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_decoder.h>
#include "elements/standard_library/policies/policy.h"

namespace streaming {

//////////////////////////////////////////////////////////////////////

const char RandomPolicy::kPolicyClassName[] = "random_policy";
const char PlaylistPolicy::kPolicyClassName[] = "playlist_policy";
const char TimedPlaylistPolicy::kPolicyClassName[] = "timed_playlist_policy";
const char OnCommandPolicy::kPolicyClassName[] = "on_command_policy";

//////////////////////////////////////////////////////////////////////

bool RandomPolicy::LoadState() {
  if ( local_state_keeper_ == NULL ) {
    LOG_WARNING << " No state keeper to Load State !!";
    return true;
  }
  string value;
  if ( local_state_keeper_->GetValue("crt", &value) ) {
    next_to_play_.push_back(value);
  }
  if ( local_state_keeper_->GetValue("next_to_play_size", &value) ) {
    int size = ::strtol(value.c_str(), NULL, 10);
    for ( int i = 0; i < size; ++i ) {
      if ( local_state_keeper_->GetValue(
             strutil::StringPrintf("next_to_play-%d", i), &value) ) {
        next_to_play_.push_back(value);
      }
    }
  }
  if ( local_state_keeper_->GetValue("history_size", &value) ) {
    int size = ::strtol(value.c_str(), NULL, 10);
    for ( int i = 0; i < size; ++i ) {
      if ( local_state_keeper_->GetValue(
             strutil::StringPrintf("history-%d", i), &value) ) {
        history_.push_back(value);
      }
    }
  }
  return true;
}

bool RandomPolicy::SaveState() {
  if ( local_state_keeper_ == NULL ) {
    LOG_WARNING << " No state keeper to Save State !!";
    return true;
  }
  local_state_keeper_->BeginTransaction();

  if ( !crt_.empty() ) local_state_keeper_->SetValue("crt", crt_);
  const int32 next_to_play_size = next_to_play_.size();
  local_state_keeper_->SetValue("next_to_play_size",
      strutil::StringPrintf("%d", next_to_play_size));
  for ( int i = 0; i < next_to_play_size; ++i ) {
    local_state_keeper_->SetValue(strutil::StringPrintf("next_to_play-%d", i),
        next_to_play_[i]);
  }

  const int32 history_size_ = history_.size();
  local_state_keeper_->SetValue("history_size",
      strutil::StringPrintf("%d", history_size_));
  for ( int i = 0; i < history_size_; ++i ) {
    local_state_keeper_->SetValue(strutil::StringPrintf("history-%d", i),
        history_[i]);
  }
  local_state_keeper_->CommitTransaction();
  return true;
}

void RandomPolicy::ClearState() {
  if ( local_state_keeper_ != NULL ) {
    local_state_keeper_->DeleteValue("crt");
    local_state_keeper_->DeleteValue("next_to_play_size");
    local_state_keeper_->DeletePrefix("next_to_play");
    local_state_keeper_->DeleteValue("history_size");
    local_state_keeper_->DeletePrefix("history");
  }
}

//////////////////////////////////////////////////////////////////////

bool PlaylistPolicy::LoadState() {
  string value;
  if ( local_state_keeper_ != NULL ) {
    if ( local_state_keeper_->GetValue("next_to_play", &value) ) {
      next_to_play_ = ::strtol(value.c_str(), NULL, 10);
    }
    if ( local_state_keeper_->GetValue("next_next_to_play", &value) ) {
      next_next_to_play_ = ::strtol(value.c_str(), NULL, 10);
    }
  }
  string encoded_playlist;
  if ( global_state_keeper_->GetValue("playlist", &encoded_playlist) ) {
    PlaylistPolicySpec playlist;
    if ( rpc::JsonDecoder::DecodeObject(encoded_playlist, &playlist) ) {
      LOG_INFO << name() << ": Loaded playlist from state";
      SetPlaylist(playlist);
    }
  } else {
    LOG_INFO << " NO KEY in playlist in: " << global_state_keeper_->prefix();
  }
  return true;
}
bool PlaylistPolicy::SaveState() {
  if ( local_state_keeper_ != NULL ) {
    local_state_keeper_->BeginTransaction();
    local_state_keeper_->SetValue("next_to_play",
                                  strutil::StringPrintf("%d", crt_));
    local_state_keeper_->SetValue("next_next_to_play",
                                  strutil::StringPrintf("%d", next_to_play_));
    local_state_keeper_->CommitTransaction();
  }
  if ( !is_temp_policy_ && global_state_keeper_ != NULL ) {
    PlaylistPolicySpec playlist;
    GetPlaylist(&playlist);
    string playlist_state;
    rpc::JsonEncoder::EncodeToString(playlist, &playlist_state);
    // VERY IMPORTANT  !! we save in local state the playlist. In this way
    // the temps do not mess the global..
    global_state_keeper_->SetValue("playlist", playlist_state);
  }
  return true;
}

void PlaylistPolicy::ClearState() {
  if ( local_state_keeper_ != NULL ) {
    local_state_keeper_->DeleteValue("next_to_play");
    local_state_keeper_->DeleteValue("next_next_to_play");
  }
  if ( !is_temp_policy_ && global_state_keeper_ != NULL ) {
    global_state_keeper_->DeleteValue("playlist");
  }
}

void PlaylistPolicy::SetPlaylist(const PlaylistPolicySpec& playlist) {
  playlist_.clear();
  for ( int i = 0; i < playlist.playlist_.get().size(); ++i ) {
    playlist_.push_back(playlist.playlist_.get()[i]);
  }
  loop_playlist_ = playlist.loop_playlist_.get();
}

void PlaylistPolicy::GetPlaylist(PlaylistPolicySpec* playlist) const {
  playlist->playlist_.ref();
  for ( int i = 0; i < playlist_.size(); ++i ) {
    playlist->playlist_.ref().push_back(playlist_[i]);
  }
  playlist->loop_playlist_.ref() = loop_playlist_;

  if ( local_state_keeper_ != NULL ) {
    if ( local_state_keeper_->timeout_ms() >= 0 ) {
      playlist->state_timeout_sec_.ref() =
          local_state_keeper_->timeout_ms()/1000;
    }
  }
}

void PlaylistPolicy::SetPlaylist(
    rpc::CallContext< MediaOpResult >* call,
    const SetPlaylistPolicySpec& playlist) {
  LOG_INFO << " Setting playlist to: " << playlist.ToString();
  if ( playlist.playlist_.is_set() ) {
    const PlaylistPolicySpec& p = playlist.playlist_.get();
    //
    // NOTE: optional media check - after some decision making we took it
    //       out
    //
    // for ( int i = 0; i < p.playlist_.get().size(); ++i ) {
    //   const streaming::Capabilities
    //       caps = element_->HasElementMedia(p.playlist_.get()[i]);
    //   if ( caps.is_invalid() ) {
    //     if ( ret.error_.get() ) {
    //       ret.description_.ref() = ret.description_.get() + ", " +
    //                                p.playlist_.get()[i];
    //     } else {
    //       ret.error_.ref() = 1;
    //       ret.description_.ref() = "Invalid Media: " +
    //                                p.playlist_.get()[i];
    //     }
    //   }
    // }
    // if ( ret.error_.get() ) {
    //   call->Complete(ret);
    //   return;
    // }

    SetPlaylist(p);
    SaveState();
  }
  if ( playlist.next_to_play_.is_set() ) {
    if ( playlist.next_to_play_.get() < 0 ||
         playlist.next_to_play_.get() >= playlist_.size() ) {
      call->Complete(MediaOpResult(false, "Invalid next to play"));
      return;
    }
    next_to_play_ = playlist.next_to_play_.get();
  } else {
    next_to_play_ = 0;
  }
  if ( playlist.switch_now_.is_set() && playlist.switch_now_.get() ) {
    GoToNext();
  }
  SaveState();
  call->Complete(MediaOpResult(true, ""));
}

void PlaylistPolicy::GetPlaylist(
    rpc::CallContext< PlaylistPolicySpec >* call) {
  PlaylistPolicySpec playlist;
  GetPlaylist(&playlist);
  call->Complete(playlist);
}

void PlaylistPolicy::GetPlayInfo(rpc::CallContext<PolicyPlayInfo>* call) {
  PolicyPlayInfo play_info;
  if ( next_to_play_ >= 0 && next_to_play_ < playlist_.size() ) {
    play_info.next_media_.ref() = playlist_[next_to_play_];
  } else {
    if ( crt_ + 1 >= playlist_.size() ) {
      if ( loop_playlist_ && !playlist_.empty() ) {
        play_info.next_media_.ref() = playlist_[0];
      }
    } else {
      play_info.next_media_.ref() = playlist_[crt_ + 1];
    }
  }
  play_info.current_media_.ref() = element_->current_media();
  call->Complete(play_info);
}

//////////////////////////////////////////////////////////////////////

bool TimedPlaylistPolicy::LoadState() {
  if ( local_state_keeper_ == NULL ) {
    LOG_WARNING << " No state keeper to Load State !!";
    return true;
  }
  string value;
  bool success = true;
  if ( local_state_keeper_->GetValue("next_to_play", &value) ) {
    next_to_play_ = ::strtol(value.c_str(), NULL, 10);
  } else { success = false; }
  if ( local_state_keeper_->GetValue("next_next_to_play", &value) ) {
    next_next_to_play_ = ::strtol(value.c_str(), NULL, 10);
  } else { success = false; }
  if ( local_state_keeper_->GetValue("last_switch_time", &value) ) {
    last_switch_time_ = ::strtoll(value.c_str(), NULL, 10);
  } else { success = false; }
  return success;
}

bool TimedPlaylistPolicy::SaveState() {
  if ( local_state_keeper_ == NULL ) {
    LOG_WARNING << " No state keeper to Save State !!";
    return true;
  }
  local_state_keeper_->BeginTransaction();
  local_state_keeper_->SetValue("next_to_play",
      strutil::StringPrintf("%d", crt_));
  local_state_keeper_->SetValue("next_next_to_play",
      strutil::StringPrintf("%d", next_to_play_));
  local_state_keeper_->SetValue("last_switch_time",
      strutil::StringPrintf("%"PRId64, last_switch_time_));
  local_state_keeper_->CommitTransaction();
  return true;
}

void TimedPlaylistPolicy::ClearState() {
  if ( local_state_keeper_ != NULL ) {
    local_state_keeper_->DeleteValue("next_to_play");
    local_state_keeper_->DeleteValue("next_next_to_play");
    local_state_keeper_->DeleteValue("last_switch_time");
  }
}

//////////////////////////////////////////////////////////////////////

bool OnCommandPolicy::Initialize() {
  bool load_success = LoadState();

  if ( rpc_server_ != NULL ) {
    is_registered_ = rpc_server_->RegisterService(local_rpc_path_, this);
    if ( is_registered_ && local_rpc_path_ != rpc_path_ ) {
      rpc_server_->RegisterServiceMirror(rpc_path_, local_rpc_path_);
    }
  }
  bool rpc_success = (is_registered_ || rpc_server_ == NULL);

  bool play_success = PlayMedia(crt_media_name_) ||
                      PlayMedia(next_media_name_) ||
                      PlayMedia(default_media_name_);

  return load_success && rpc_success && play_success;
}

void OnCommandPolicy::Reset() {
}

bool OnCommandPolicy::NotifyEos() {
  string media = (next_media_name_ != "" ?
                  next_media_name_ : default_media_name_);
  next_media_name_ = "";
  return PlayMedia(media);
}

bool OnCommandPolicy::NotifyTag(const Tag* tag, int64 timestamp_ms) {
  return true;
}

string OnCommandPolicy::GetPolicyConfig() {
  return "";
}

bool OnCommandPolicy::LoadState() {
  if ( global_state_keeper_ == NULL ||
       !global_state_keeper_->GetValue("default_media_name",
                                       &default_media_name_) ) {
    LOG_ERROR << "Failed to LoadState from global state keeper";
    return false;
  }
  if ( local_state_keeper_ != NULL ) {
    local_state_keeper_->GetValue("crt", &crt_media_name_);
    local_state_keeper_->GetValue("next", &next_media_name_);
  }
  return true;
}

bool OnCommandPolicy::SaveState() {
  if ( !global_state_keeper_->SetValue("default_media_name",
                                        default_media_name_) ) {
    LOG_ERROR << "Failed to SaveState to global state keeper";
    return false;
  }
  if ( local_state_keeper_ != NULL ) {
    local_state_keeper_->SetValue("crt", crt_media_name_);
    local_state_keeper_->SetValue("next", next_media_name_);
  }
  return true;
}

void OnCommandPolicy::ClearState() {
  if ( global_state_keeper_ != NULL ) {
    global_state_keeper_->DeleteValue("default_media_name");
  }
  if ( local_state_keeper_ != NULL ) {
    local_state_keeper_->DeleteValue("crt");
    local_state_keeper_->DeleteValue("next");
  }
}

bool OnCommandPolicy::PlayMedia(const string& media) {
  if ( media == "" ) {
    return false;
  }
  if ( !element_->SwitchCurrentMedia(media, NULL, true) ) {
    LOG_ERROR << "Failed to switch element to media: [" << media << "]";
    return false;
  }
  crt_media_name_ = media;
  SaveState();
  return true;
}

void OnCommandPolicy::SwitchPolicy(
    rpc::CallContext< MediaOpResult >* call,
    const string& media_name,
    bool set_as_default,
    bool also_switch) {
  //
  // NOTE: optional media check - after some decision making we took it
  //       out
  //
  // streaming::Capabilities
  // caps = element_->HasElementMedia(media_name->CStr());
  // if ( caps.is_invalid() ) {
  //   ret.error_.ref() = 1;
  //   ret.description_.ref() = "Invalid Media: " + media_name->StdStr();
  //   call->Complete(ret);
  // }
  //
  if ( set_as_default ) {
    default_media_name_ = media_name;
    next_media_name_ = media_name;
  }
  if ( also_switch ) {
    PlayMedia(media_name);
  } else {
    if ( !set_as_default ) {
      next_media_name_ = media_name;
    }
  }
  SaveState();
  call->Complete(MediaOpResult(true, ""));
}

void OnCommandPolicy::GetDefaultMedia(rpc::CallContext< string >* call) {
  call->Complete(default_media_name_);
}

void OnCommandPolicy::GetPlayInfo(rpc::CallContext<PolicyPlayInfo>* call) {
  PolicyPlayInfo play_info;
  play_info.next_media_ =
      next_media_name_.empty() ? default_media_name_ : next_media_name_;
  play_info.current_media_ = element()->current_media();
  call->Complete(play_info);
}

//////////////////////////////////////////////////////////////////////
}
