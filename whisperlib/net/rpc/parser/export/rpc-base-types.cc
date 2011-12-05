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

#include "net/rpc/parser/export/rpc-base-types.h"

namespace  {
static vector<string> kBaseTypeArray;
static pthread_once_t glb_once_control = PTHREAD_ONCE_INIT;
void BuildRpcBaseTypeArray() {
  kBaseTypeArray.push_back(string(RPC_VOID));
  kBaseTypeArray.push_back(string(RPC_BOOL));
  kBaseTypeArray.push_back(string(RPC_INT));
  kBaseTypeArray.push_back(string(RPC_BIGINT));
  kBaseTypeArray.push_back(string(RPC_FLOAT));
  kBaseTypeArray.push_back(string(RPC_STRING));
  kBaseTypeArray.push_back(string(RPC_ARRAY));
  kBaseTypeArray.push_back(string(RPC_MAP));
}
}

namespace rpc {
const vector<string>& BaseTypeArray() {
  pthread_once(&glb_once_control, BuildRpcBaseTypeArray);
  return kBaseTypeArray;
}
}
