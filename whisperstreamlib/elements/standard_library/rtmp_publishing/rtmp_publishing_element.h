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

#ifndef __MEDIA_BASE_MEDIA_RTMP_PUBLISHING_ELEMENT_H__
#define __MEDIA_BASE_MEDIA_SWITCHING_ELEMENT_H__

#include <string>
#include <vector>
#include <whisperlib/common/base/types.h>
#include WHISPER_HASH_SET_HEADER
#include <whisperstreamlib/base/element.h>
#include <whisperstreamlib/base/callbacks_manager.h>
#include <whisperstreamlib/flv/flv_tag_splitter.h>
#include <whisperlib/net/rpc/lib/server/rpc_http_server.h>
#include <whisperlib/common/io/checkpoint/state_keeper.h>
#include <whisperstreamlib/elements/standard_library/http_server/import_element.h>

#include <whisperstreamlib/elements/standard_library/auto/standard_library_invokers.h>

namespace streaming {

///////////////////////////////////////////////////////////////////////
// Interface for streams that come to us via rtmp
//

class RtmpPublishingElement : public ImportElement,
                              public ServiceInvokerRtmpPublishingElementService {
 public:
  static const char kElementClassName[];
  RtmpPublishingElement(const string& name,
                        ElementMapper* mapper,
                        net::Selector* selector,
                        const string& rpc_path,
                        rpc::HttpServer* rpc_server,
                        io::StateKeepUser* state_keeper,
                        const string& authorizer_name);
  virtual ~RtmpPublishingElement();

  using ImportElement::AddImport;
  using ImportElement::DeleteImport;

  //////////////////////////////////////////////////////////////////////////
  // overridden ImportElement methods
  virtual bool Initialize();
 protected:
  virtual ImportElementData* CreateNewImport(const string& import_name);

  /////////////////////////////////////////////////////////////////////////
  // methods from ServiceInvokerRtmpPublishingElementService
  void AddImport(rpc::CallContext<MediaOpResult>* call, const string& name);
  void DeleteImport(rpc::CallContext<rpc::Void>* call, const string& name);
  void GetImports(rpc::CallContext< vector<string> >* call);

 private:
  // just for logging
  const string info_;

  DISALLOW_EVIL_CONSTRUCTORS(RtmpPublishingElement);
};
}

#endif  // __MEDIA_BASE_MEDIA_RTMP_PUBLISHING_ELEMENT_H__
