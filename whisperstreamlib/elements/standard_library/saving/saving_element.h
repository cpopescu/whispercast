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

#ifndef __MEDIA_BASE_MEDIA_SAVING_ELEMENT_H__
#define __MEDIA_BASE_MEDIA_SAVING_ELEMENT_H__

#include <string>
#include <whisperlib/common/base/alarm.h>
#include <whisperstreamlib/base/saver.h>
#include <whisperstreamlib/base/element.h>

namespace streaming {

///////////////////////////////////////////////////////////////////////
//
// SavingElement
//
// An element that saves the incoming stream as a bunch of .part files,
// compatible with TimeSavingElement.
// This element only receives tags, you cannot register by "AddRequest".
// It's a deadline (just like an exporter).
//
class SavingElement : public Element {
 public:
  static const char kElementClassName[];
  static const uint64 kReconnectDelay;

 public:
  // media_dir: all saved media goes under this directory
  // save_dir: relative path under media_dir.
  //           We actually save data under: media_dir/save_dir
  SavingElement(const string& name,
                ElementMapper* mapper,
                net::Selector* selector,
                const string& base_media_dir,
                const string& media,
                const string& save_dir);
  virtual ~SavingElement();

  ////////////////////////////////////////////////////////////////////////
  // Element interface methods
  virtual bool Initialize();
  virtual bool AddRequest(const string& media, Request* req,
                          ProcessingCallback* callback);
  virtual void RemoveRequest(streaming::Request* req);
  virtual bool HasMedia(const string& media);
  virtual void ListMedia(const string& media_dir, vector<string>* out);
  virtual bool DescribeMedia(const string& media, MediaInfoCallback* callback);
  virtual void Close(Closure* call_on_close);

 private:
  // tag receiver. All tags are sent to the "saver_";
  void ProcessTag(const Tag* tag, int64 timestamp_ms);

  // starts the internal request to "media_".
  // We receive tags on this request, and send them to the saver_.
  void OpenMedia();
  void CloseMedia();

 private:
  net::Selector* selector_;
  const string base_media_dir_;
  const string media_;
  const string save_dir_;

  // we register this request in order to receive tags through ProcessTag()
  streaming::Request* internal_req_;

  // permanent callback to ProcessTag
  ProcessingCallback* process_tag_;

  Saver* saver_;

  // try to OpenMedia again
  util::Alarm open_media_alarm_;

  DISALLOW_EVIL_CONSTRUCTORS(SavingElement);
};

}

#endif  // __MEDIA_BASE_MEDIA_SAVING_ELEMENT_H__
