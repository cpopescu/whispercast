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
// Authors: Catalin Popescu

// Simple utility that prints all the tags from a given http media file.

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/net/base/selector.h>

#include "elements/standard_library/http_client/http_client_element.h"

//////////////////////////////////////////////////////////////////////

DEFINE_string(hostip,
              "",
              "Host IP to tag");
DEFINE_int32(port,
             80,
             "Port to connect");
DEFINE_string(url_path,
              "",
              "Url to tag");

//////////////////////////////////////////////////////////////////////

void PrintTag(net::Selector* selector, const streaming::Tag* media_tag) {
  if ( media_tag->type() == streaming::Tag::TYPE_EOS ) {
    selector->MakeLoopExit();
  } else {
    printf("TAG : %s\n", media_tag->ToString().c_str());
  }
}
int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  CHECK(!FLAGS_hostip.empty());
  CHECK(!FLAGS_url_path.empty());
  net::Selector selector;
  streaming::HttpClientElement src("test", "test", NULL, &selector, NULL, NULL);
  src.AddElement("httpget",
                 FLAGS_hostip.c_str(), FLAGS_port,
                 FLAGS_url_path.c_str(),
                 false, false, streaming::Tag::kAnyType, NULL);
  streaming::Request req;
  if ( src.AddRequest("test/httpget", &req,
                      NewPermanentCallback(PrintTag, &selector)) ) {
    selector.Loop();
  }
  LOG_INFO << "Select loop ended !!";
}
