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

#ifndef __NET_RPC_PARSER_EXPORT_RPC_EXPORTER_H__
#define __NET_RPC_PARSER_EXPORT_RPC_EXPORTER_H__

#include <map>
#include <string>
#include <vector>
#include <set>
#include <whisperlib/common/base/types.h>
#include <whisperlib/net/rpc/parser/rpc-ptypes.h>
#include <whisperlib/net/rpc/parser/common.h>

namespace rpc {

class Exporter {
 public:
  Exporter(const string& languageName,
           const PCustomTypeArray& customTypes,
           const PServiceArray& services,
           const PVerbatimArray& verbatim,
           const vector<string>& inputFilenames);
  virtual ~Exporter();

  // returns: "C++", "Javascript", "PHP" ..
  const string& GetLanguageName() const;

  // Returns the restricted keywords for this specific language.
  // e.g. for C++: void, int, bool, string, unsigned, for, while, switch, if, ..
  // The set is used to do an early (immediate after parse) check of type names
  // correctness.
  virtual const Keywords& GetRestrictedKeywords() const = 0;

  // Pairs of [property-name, value]. These are exporter specific parameters.
  // e.g. for the C++ exporter the map should include:
  //        fclientTypes = <path to client rpc-types(.h .cc)>
  //        fclientWrappers = <path to client rpc-wrappers(.h .cc)>
  //        fserverTypes = <path to server rpc-types(.h .cc)>
  //        fserverMethods = <path to server rpc-services-methods.h>
  //        fserverImpl = <path to server rpc-services-impl(.h .cc)>
  virtual bool SetParams(const map<string, string>& paramValues) = 0;

  //  Export types and services into a custom programming language.
  // returns:
  //  - true on success.
  //  - false on internal error. Usually undefined types or illegal type names.
  virtual bool Export() = 0;

  // Gets the all verbatims for this language
  string GetVerbatim() const;
 protected:
  const string languageName_;
  const PCustomTypeArray& customTypes_;
  const PServiceArray& services_;
  const PVerbatimArray& verbatim_;
  // just for logging into generated files:
  const vector<string>& inputFilenames_;
};
}
#endif   // __NET_RPC_PARSER_EXPORT_RPC_EXPORTER_H__
