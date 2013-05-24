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

#include "rtmp/objects/rtmp_objects.h"
#include "rtmp/events/rtmp_event_notify.h"
#include "rtmp/objects/amf/amf_util.h"
#include "rtmp/objects/amf/amf0_util.h"

namespace rtmp {

AmfUtil::ReadStatus EventNotify::DecodeBody(io::MemoryStream* in,
    AmfUtil::Version v) {
  Clear();
  const AmfUtil::ReadStatus err = name_.Decode(in, rtmp::AmfUtil::AMF0_VERSION);
  if ( err != rtmp::AmfUtil::READ_OK ) {
    return err;
  }
  while ( !in->IsEmpty() ) {
    CObject* obj = NULL;
    const AmfUtil::ReadStatus err = Amf0Util::ReadNextObject(in, &obj);
    if ( err != AmfUtil::READ_OK ) {
      return err;
    }
    values_.push_back(obj);
  }
  return AmfUtil::READ_OK;
}

void EventNotify::EncodeBody(io::MemoryStream* out, AmfUtil::Version v) const {
  name_.Encode(out, v);
  for ( int i = 0; i < values_.size(); ++i ) {
    values_[i]->Encode(out, v);
  }
}
}
