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

#ifndef __MEDIA_BASE_MEDIA_COMMAND_SOURCE_H__
#define __MEDIA_BASE_MEDIA_COMMAND_SOURCE_H__

#include <string>
#include <vector>
#include <map>
#include <whisperlib/net/base/selector.h>
#include <whisperstreamlib/base/element.h>
#include <whisperstreamlib/base/consts.h>
#include <whisperstreamlib/base/tag_splitter.h>

namespace streaming {
class CommandElementData;

///////////////////////////////////////////////////////////////////////
//
// CommandElement
//
// streaming::Element implementation that runs commands w/ a name, and pipe
// the output of those commands as media content (through a splitter of
// your choice)..
//
class CommandElement : public Element {
 public:
  CommandElement(const string& name,
                 ElementMapper* mapper,
                 net::Selector* selector);
  virtual ~CommandElement();


  // streaming::Element interface
  virtual bool Initialize() { return true; }
  virtual bool AddRequest(const string& media, Request* req,
                          ProcessingCallback* callback);
  virtual void RemoveRequest(Request* req);
  virtual bool HasMedia(const string& media);
  virtual void ListMedia(const string& media_dir, vector<string>* out);
  virtual bool DescribeMedia(const string& media, MediaInfoCallback* callback);
  virtual void Close(Closure* call_on_close);

  static const char kElementClassName[];

  // Initializes element names w/ commands
  bool AddElement(const string& name, MediaFormat media_format,
                  const string& command, bool should_reopen);

 protected:
  // Finds if we can provide the givven name element
  CommandElementData* GetElement(const string& name);

 private:
  // Callback when a pipe is closed
  void ClosedElementNotification(CommandElementData* data);

 private:
  net::Selector* const selector_;
  int num_registered_callbacks_;
  typedef map<string, CommandElementData*> ElementMap;
  ElementMap elements_;
  typedef map<streaming::Request*, CommandElementData*> RequestMap;
  RequestMap requests_;

  Closure* close_completed_;

  DISALLOW_EVIL_CONSTRUCTORS(CommandElement);
};
}

#endif
