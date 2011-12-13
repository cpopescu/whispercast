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


#ifndef __MEDIA_BASE_MEDIA_SWITCHING_ELEMENT_H__
#define __MEDIA_BASE_MEDIA_SWITCHING_ELEMENT_H__

#include <string>
#include <vector>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/alarm.h>
#include <whisperlib/net/base/selector.h>
#include <whisperstreamlib/base/element.h>
#include <whisperstreamlib/base/tag_distributor.h>
#include <whisperlib/net/rpc/lib/server/rpc_http_server.h>
#include "elements/standard_library/auto/standard_library_invokers.h"

#include <whisperstreamlib/base/tag_normalizer.h>

namespace streaming {

///////////////////////////////////////////////////////////////////////
//
// SwitchingElement
//
// An element that switches from one element to another when
// a command is called.  It is a policy driven element that can serve media
// only under our name. (Basically the policy changes what underneath media
// to server under a name that matches this SwitchingElement's name)
class SwitchingElement :
      public PolicyDrivenElement,
      public ServiceInvokerSwitchingElementService {
 public:
  // We call your provided callback when our current element is exhausted.
  // Return true to maintain this element (and the implicit added callbacks)
  typedef ResultClosure<bool> EmptyCallback;

  static const char kElementClassName[];
  // we don't register play timeouts faster than this time
  static const int64 kTagTimeoutRegistrationGracePeriodMs = 1000;
  // minimum time interval between two consecutive switches.
  // Useful when the upstream signals EOS too fast (e.g. missing file)
  static const int64 kRegisterMinIntervalMs = 3000;

 public:
  // Constructs a SwitchingElement - we don NOT own empty callback !
  // params:
  // default_flavour_id:
  //    Original comment: When we do not have a flavor upstream,
  //                      we use this flavour to serve tags.
  //    Currently: this has no use.
  //               Upstream registration has a flavour mask in caps.
  //               Clients specify a flavour mask in the req.
  //               What was this intended for!?
  // save_media_bootstrap:
  //    Original comment: bootstrap also media tags ? (normal just for global
  //                      else costs too much and does not make sense)
  //    Currently: not used.
  SwitchingElement(const char* name,
                   const char* id,
                   ElementMapper* mapper,
                   net::Selector* selector,
                   const char* rpc_path,
                   rpc::HttpServer* rpc_server,
                   const streaming::Capabilities& caps,
                   int default_flavour_id,
                   int32 tag_timeout_ms,
                   int32 write_ahead_ms,
                   bool media_only_when_used,
                   bool save_media_bootstrap);
  virtual ~SwitchingElement();

  net::Selector* selector() const {
    return selector_;
  }
  bool is_registered() const {
    return current_media_ != "";
  }

  // returns the number of downstream clients
  uint32 CountClients() const;

  // Register our processing callback upstream on media_name_to_register_ path.
  void Register(string media_name);
  // Removes the upstream link. We stop receiving tags.
  // If not registered, unregistering has no effect.
  void Unregister(bool send_source_ended, bool send_flush);

  ///////////////////////////////////////////////////////////////////
  // PolicyDrivenElement interface methods
  virtual bool SwitchCurrentMedia(const string& media_name,
                                  const RequestInfo* req_info,
                                  bool force_switch);
  virtual const string& current_media() const {
    return current_media_;
  }

  ////////////////////////////////////////////////////////////////////
  // ServiceInvokerSwitchingElementService interface methods
  virtual void GetCurrentMedia(rpc::CallContext<string>* call) {
    call->Complete(current_media_);
  }

 protected:
  // Gives all downstream clients an EOS.
  // The order has to be: first Unregister() then CloseAllClients().
  // So that clients can get the SourceEnded tag.
  void CloseAllClients(bool forced);

  // Processing function - we call from this our underneath processing
  // callbacks
  void ProcessTag(const Tag* tag, int64 timestamp_ms);

  // We call this internally when the current element signals sends us an EOS.
  void StreamEnded();

  // We timeout on no tag received for a while..
  void TagReceiveTimeout();

 public:
  ///////////////////////////////////////////////////////////////////////
  // The Element interface methods
  virtual bool Initialize();
  virtual bool AddRequest(const char* media, streaming::Request* req,
                          streaming::ProcessingCallback* callback);
  virtual void RemoveRequest(streaming::Request* req);
  virtual bool HasMedia(const char* media, Capabilities* out);
  virtual void ListMedia(const char* media_dir, ElementDescriptions* medias);
  virtual bool DescribeMedia(const string& media, MediaInfoCallback* callback);
  virtual void Close(Closure* call_on_close);

 private:
  // Completely close this element. This function is a delegate from Close()
  // because we need to decouple Close() from the main call.
  void InternalClose();

  // Possibly registers the tag timeout callback (if we did not do it
  // too recently).
  void MaybeReregisterTagTimeout(bool force);

 protected:
  // We use this caps as defaults when we register ourselves..
  const streaming::Capabilities caps_;
  // We timeout at this value and force switch upon receiving no tags
  const int32 tag_timeout_ms_;
  // We keep the stream this much ahead of the real time
  const int32 write_ahead_ms_;
  // We use this to register ourselves
  streaming::Request* req_;
  streaming::RequestInfo* req_info_;

  net::Selector* const selector_;
  const string rpc_path_;
  rpc::HttpServer* const rpc_server_;

  // true => Auto Unregister() when there are no clients.
  // false => Keep registered always.
  const bool media_only_when_used_;

  // Name of the upstream media stream.
  // The element is unique but it can serve multiple streams.
  string current_media_;
  // While we have no callbacks registered we don't register to new media
  string media_name_to_register_;

  // tag normalizer for the input of the switching element
  streaming::TagNormalizer normalizer_;

  // permanent callback to ProcessTag(..)
  streaming::ProcessingCallback* process_tag_callback_;

  // Tag timeout alarm
  util::Alarm tag_timeout_alarm_;

  // when we last registered the tag_timeout_callback_ alarm
  // (so we don't overdo it .. :)
  int64 last_tag_timeout_registration_time_;

  // the downstream clients
  streaming::TagDistributor* distributors_[kNumFlavours];

  // Run Register through an alarm. This way we can delay Register.
  util::Alarm register_alarm_;
  // Separate StreamEnded() call context
  util::Alarm stream_ended_alarm_;

  // external closure, used to signal that asynchronous Close(..) completed
  Closure* close_completed_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(SwitchingElement);
};
}

#endif  // __MEDIA_BASE_MEDIA_SWITCHING_ELEMENT_H__
