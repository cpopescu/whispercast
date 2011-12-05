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

#ifndef __NET_RPC_PARSER_RPC_PTYPES_CHECK_H__
#define __NET_RPC_PARSER_RPC_PTYPES_CHECK_H__

#include <map>
#include <set>
#include <string>

#include <whisperlib/common/base/types.h>
#include <whisperlib/net/rpc/parser/rpc-ptypes.h>
#include <whisperlib/net/rpc/parser/common.h>

//  This method verifies types definitions correctness.
//    - defined types and methods cannot be a restricted keyword
//    - you cannot use a type before it's definition.
//    - you cannot re-define an existing type.
//    - methods arguments must be known types (basic types or
//      previously defined)
// input:
//  customTypes: the custom types defined by user.
//  services: the custom services defined by user.
//  restrictedKeywords: restricted keywords for a specific programming language.
//                      These words cannot be variable name or method name.
//                 e.g. for C++:
//                    {void, int, chat, bool, unsigned, for, while, ..}
// returns:
//  true = all type definitions are correct. There are no undefined types used.
//  false = errors were found: undefined types, redefined types or
//          keywords defined as types.
bool VerifyPTypesCorrectenss(const PCustomTypeArray customTypes,
                             const PServiceArray services,
                             const Keywords& restrictedKeywords);

#endif  // __NET_RPC_PARSER_RPC_PTYPES_CHECK_H__
