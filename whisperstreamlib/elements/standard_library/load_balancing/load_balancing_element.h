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

#include <string>

#include <whisperlib/common/base/types.h>
#include WHISPER_HASH_SET_HEADER
#include <whisperstreamlib/base/element.h>

#ifndef __MEDIA_ELEMENTS_LOAD_BALANCING_ELEMENT_H__
#define __MEDIA_ELEMENTS_LOAD_BALANCING_ELEMENT_H__

namespace streaming {

class LoadBalancingElement : public Element {
 public:
  LoadBalancingElement(const string& name,
                       ElementMapper* mapper,
                       net::Selector* selector,
                       const vector<string>& sub_elements);
  virtual ~LoadBalancingElement();

  static const char kElementClassName[];

  // Element Interface:
  virtual bool Initialize();
  virtual bool AddRequest(const string& media, Request* req,
                          ProcessingCallback* callback);
  virtual void RemoveRequest(streaming::Request* req);
  virtual bool HasMedia(const string& media);
  virtual void ListMedia(const string& media_dir, vector<string>* out);
  virtual bool DescribeMedia(const string& media, MediaInfoCallback* callback);
  virtual void Close(Closure* call_on_close);

 private:
  int32 next_element_;
  vector<string> sub_elements_;
  struct ReqStruct {
    streaming::ProcessingCallback* callback_;
    string new_element_name_;
    streaming::ProcessingCallback* added_callback_;
    bool eos_received_;
    ReqStruct(streaming::ProcessingCallback* callback)
        : callback_(callback),
          added_callback_(NULL),
          eos_received_(false) {
    }
    ~ReqStruct() {
      delete added_callback_;
    }
  };
  void ProcessTag(ReqStruct* rs, const streaming::Tag* tag, int64 timestamp_ms);

  typedef hash_map<Request*, ReqStruct*> ReqMap;
  ReqMap req_map_;

  net::Selector* selector_;

  // called when asynchronous Close completes
  Closure* close_completed_;

  DISALLOW_EVIL_CONSTRUCTORS(LoadBalancingElement);
};
}
#endif  // __MEDIA_ELEMENTS_LOAD_BALANCING_ELEMENT_H__
