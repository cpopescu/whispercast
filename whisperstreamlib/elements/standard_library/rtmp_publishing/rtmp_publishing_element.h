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
class RtmpPublishingData;

///////////////////////////////////////////////////////////////////////
//
// RtmpPublishingElement -
//
// Interface for streams that come to us via rtmp
//

class RtmpPublishingElement
    : public ImportElement<ServiceInvokerRtmpPublishingElementService,
                           RtmpPublishingData,
                           RtmpPublishingElementDataSpec> {
  typedef ImportElement<ServiceInvokerRtmpPublishingElementService,
                        RtmpPublishingData,
                        RtmpPublishingElementDataSpec> BaseClass;
 public:
  // Constructs a SwitchingElement - we don NOT own empty callback !
  RtmpPublishingElement(const string& name,
                        const string& id,
                        ElementMapper* mapper,
                        net::Selector* selector,
                        const string& media_dir,
                        const string& rpc_path,
                        rpc::HttpServer* rpc_server,
                        io::StateKeepUser* state_keeper,
                        streaming::SplitterCreator* splitter_creator,
                        const string& authorizer_name);
  virtual ~RtmpPublishingElement();

  static const char kElementClassName[];

  // some overridden streaming::Element interface methods
  virtual bool Initialize();
  virtual void Close(Closure* call_on_close);

 protected:
  virtual RtmpPublishingData* CreateNewImport(const char* import_name,
                                              const char* full_name,
                                              Tag::Type tag_type,
                                              const char* save_dir,
                                              bool append_only,
                                              bool disable_preemption,
                                              int32 prefill_buffer_ms,
                                              int32 advance_media_ms,
                                              int32 buildup_interval_sec,
                                              int32 buildup_delay_sec);
 private:
  // just for logging
  const string info_;

  DISALLOW_EVIL_CONSTRUCTORS(RtmpPublishingElement);
};
}

#endif  // __MEDIA_BASE_MEDIA_RTMP_PUBLISHING_ELEMENT_H__
