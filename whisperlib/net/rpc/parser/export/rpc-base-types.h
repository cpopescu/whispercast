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

#ifndef __NET_RPC_PARSER_EXPORT_RPC_BASE_TYPES_H__
#define __NET_RPC_PARSER_EXPORT_RPC_BASE_TYPES_H__

#include <string>
#include <vector>

#include <whisperlib/common/base/types.h>

// These are the base rpc types. These types must have a correspondent in
// all supported translator languages.
//
// IMPORTANT: Want to add new base types?
//   then remember to UPDATE: rpc-base-types.cc:   RPCBaseTypeArray()
//   and every:               rpc-exporter-XXX.cc: GetBaseTypeCorrespondence()
#define RPC_VOID "void"
#define RPC_BOOL "bool"
#define RPC_INT "int"
#define RPC_BIGINT "bigint"
#define RPC_FLOAT "float"
#define RPC_STRING "string"
#define RPC_ARRAY "array"
#define RPC_MAP "map"

// Returns an array of all RPC basic types (defined above).
namespace rpc {
const vector<string>& BaseTypeArray();
}

#endif   // __NET_RPC_PARSER_EXPORT_RPC_BASE_TYPES_H__
