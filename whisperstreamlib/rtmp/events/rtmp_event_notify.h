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

#ifndef __NET_RTMP_EVENTS_RTMP_EVENT_NOTIFY_H__
#define __NET_RTMP_EVENTS_RTMP_EVENT_NOTIFY_H__

#include <string>
#include <vector>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperstreamlib/rtmp/events/rtmp_event.h>
#include <whisperstreamlib/rtmp/events/rtmp_call.h>
#include <whisperstreamlib/rtmp/objects/rtmp_objects.h>

namespace rtmp {

class EventNotify : public Event {
 public:
  explicit EventNotify(const Header& header)
      : Event(header, EVENT_NOTIFY, SUBTYPE_SERVICE_CALL) {
  }
  virtual ~EventNotify() {
    Clear();
  }

  const CString& name() const {
    return name_;
  }
  void set_name(const string& s) {
    name_.mutable_value()->assign(s);
  }
  void add_value(CObject* v) {
    values_.push_back(v);
  }
  const vector<CObject*>& values() const {
    return values_;
  }
  virtual string ToStringAttr() const {
    string s = " Name: [" + name_.ToString() + "]\n";
    for ( int i = 0; i < values_.size(); ++i ) {
      s += "  Value: " + values_[i]->ToString();
    }
    return s;
  }

  virtual AmfUtil::ReadStatus DecodeBody(io::MemoryStream* in,
                                         AmfUtil::Version v);
  virtual void EncodeBody(io::MemoryStream* out,
                          AmfUtil::Version v) const;

 protected:
  CString name_;
  vector<CObject*> values_;

  void Clear() {
    for ( int i = 0; i < values_.size(); ++i ) {
      delete values_[i];
    }
    values_.clear();
    name_.mutable_value()->clear();
  }
};
}

#endif  // __NET_RTMP_EVENTS_RTMP_EVENT_NOTIFY_H__
