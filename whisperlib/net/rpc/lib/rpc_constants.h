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

#ifndef __NET_RPC_LIB_RPC_CONSTANTS_H__
#define __NET_RPC_LIB_RPC_CONSTANTS_H__

#include <string>
#include <whisperlib/common/base/types.h>

namespace rpc {

// 2 uses:
// - internal type (both JsonEncoder and JsonDecoder have type: kCodecIdJson)
// - codec id in binary protocol (e.g. in TCP protocol)
enum CodecId {
  kCodecIdBinary = 1,
  kCodecIdJson = 2,
};

// for textual representation of the codec (e.g. in HTTP protocol)
extern const string kCodecNameJson;
extern const string kCodecNameBinary;
const string& CodecName(CodecId id);
bool GetCodecIdFromName(const string& codec_name, CodecId* out_codec_id);

// beware of the normalization of http fields: content-lengTH -> Content-Length
extern const string kHttpFieldCodec;
extern const string kHttpFieldParams;

}


#endif  // __NET_RPC_LIB_RPC_CONSTANTS_H__
