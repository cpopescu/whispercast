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
//

#ifndef __MEDIA_ELEMENTS_REDIRECTING_ELEMENT_H__
#define __MEDIA_ELEMENTS_REDIRECTING_ELEMENT_H__

#include <string>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/re.h>
#include <whisperstreamlib/base/element.h>

namespace streaming {
///////////////////////////////////////////////////////////////////////
//
// RedirectingElement
//
// An element that matches incoming request path to an internal map of
// regular expressions.
// If a match is found, we change request path: by replacing our element name
// with the map value. Then the request is forwarded to the ElementMapper.
// We also have to change on the fly the TYPE_SOURCE_STARTED and
// TYPE_SOURCE_ENDED tags.
//
// e.g. Internal map:
//        "\.flv$" => "aio_files_flv"
//        "\.f4v$" => "f4v_to_flv_convertor/aio_files_f4v"
//
//      A request comes with path: "redirect/media.flv"
//      we change it to: "aio_files_flv/media.flv"
//
//      Another request comes with path: "redirect/media.f4v"
//      we change it to: "f4v_to_flv_convertor/aio_files_f4v/media.f4v"
//
class RedirectingElement : public Element {
 public:
  static const char kElementClassName[];
  RedirectingElement(const string& name,
                     ElementMapper* mapper,
                     const map<string, string>& redirection);
  virtual ~RedirectingElement();

 private:
  struct ReqStruct;
  void ProcessTag(ReqStruct* rs, const streaming::Tag* tag, int64 timestamp_ms);

 public:
  ///////////////////////////////////////////////////////////////////////
  // Element methods
  virtual bool Initialize();
  virtual bool AddRequest(const string& media, Request* req,
                          ProcessingCallback* callback);
  virtual void RemoveRequest(streaming::Request* req);
  virtual bool HasMedia(const string& media);
  virtual void ListMedia(const string& media_dir, vector<string>* out);
  virtual bool DescribeMedia(const string& media, MediaInfoCallback* callback);
  virtual void Close(Closure* call_on_close);

 private:
  struct Redirection {
    re::RE* re_;
    string value_;
    Redirection(re::RE* re, const string& value)
      : re_(re), value_(value) {
    }
    ~Redirection() {
      delete re_;
      re_ = NULL;
    }
  };
  typedef vector<Redirection*> RedirectionArray;
  RedirectionArray redirections_;

  struct ReqStruct {
    // we get this callback from the mapper
    streaming::ProcessingCallback* downstream_callback_;
    // this is a permanent callback to ProcessTag()
    streaming::ProcessingCallback* our_callback_;
    const string original_media_;
    const string redirection_path_;
    ReqStruct(streaming::ProcessingCallback* downstream_callback,
              streaming::ProcessingCallback* our_callback,
              const string& original_media,
              const string& redirection_path)
        : downstream_callback_(downstream_callback),
          our_callback_(NULL),
          original_media_(original_media),
          redirection_path_(redirection_path) {
    }
    ~ReqStruct() {
      delete our_callback_;
      our_callback_ = NULL;
    }
  };
  typedef map<Request*, ReqStruct*> ReqMap;
  ReqMap req_map_;

  DISALLOW_EVIL_CONSTRUCTORS(RedirectingElement);
};
} // streaming

#endif  // __MEDIA_ELEMENTS_REDIRECTING_ELEMENT_H__
