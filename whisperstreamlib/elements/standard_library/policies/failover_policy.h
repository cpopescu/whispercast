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
#ifndef __STREAMING_ELEMENTS_FAILOVER_POLICY_H__
#define __STREAMING_ELEMENTS_FAILOVER_POLICY_H__

// Here is the deal: you may want to have a source of media that comes from
// one place (say a live event or something) but when that is not available
// you want to play something else (say some clips). In that case this policy
// comes to help. We default to the first media when that is available, and
// when is not, we go to the second. When first media becomes available, whe
// switch back to that.
//
// Other uses may include building CDN-s w/ whispercast..
//

#include <string>

#include <whisperlib/net/base/selector.h>
#include <whisperstreamlib/base/element.h>
#include <whisperstreamlib/elements/standard_library/auto/standard_library_invokers.h>

namespace streaming {

class FailoverPolicy : public Policy {
 public:
  FailoverPolicy(const char* name,
                 PolicyDrivenElement* element,
                 ElementMapper* mapper,
                 net::Selector* selector,
                 const string& main_media,
                 const string& failover_media,
                 int32 main_media_tags_received_switch_limit,
                 int32 failover_timeout,
                 bool change_to_main_only_on_switch);
  virtual ~FailoverPolicy();
  // Policy interface:
  virtual bool Initialize();
  virtual bool NotifyEos();
  virtual bool NotifyTag(const Tag* tag);
  virtual void Reset();
  virtual string GetPolicyConfig();

  static const char kPolicyClassName[];
 private:
  void OpenMedia();
  void CloseMedia();
  void ProcessMainTag(const Tag* tag);
  void TagReceiveTimeout();
  bool MaybeReregisterTagTimeout(bool force, int32 tag_timeout);

  ElementMapper* const mapper_;
  net::Selector* const selector_;
  const string main_media_;
  const string failover_media_;
  const int64 main_media_tags_received_switch_limit_;
  const int32 failover_timeout_ms_;
  const bool change_to_main_only_on_switch_;
  string current_media_;
  int64 main_media_tags_received_;

  streaming::Request* internal_req_;
  ProcessingCallback* process_tag_callback_;

  Closure* open_media_callback_;
  Closure* tag_timeout_callback_;

  int64 last_tag_timeout_registration_time_;
  static const int kTagTimeoutRegistrationGracePeriodMs = 500;
  static const int kRetryOpenMediaTimeMs = 2500;


  DISALLOW_EVIL_CONSTRUCTORS(FailoverPolicy);
};
}

#endif  // __STREAMING_ELEMENTS_FAILOVER_POLICY_H__
