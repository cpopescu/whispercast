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

#ifndef __NET_RPC_PARSER_EXPORT_JAVASCRIPT_RPC_EXPORTER_JS_H__
#define __NET_RPC_PARSER_EXPORT_JAVASCRIPT_RPC_EXPORTER_JS_H__

#include <map>
#include <string>
#include <vector>

#include <whisperlib/net/rpc/parser/export/rpc-exporter.h>

namespace rpc {

class ExporterJS : public Exporter {
 public:
  ExporterJS(const PCustomTypeArray& customTypes,
             const PServiceArray& services,
             const PVerbatimArray& verbatim,
             const vector<string>& inputFilenames);
  virtual ~ExporterJS();

  //////////////////////////////////////////////////////////////////////
  //
  //                    Exporter interface methods
  //

  const Keywords& GetRestrictedKeywords() const;

  // Pairs of [property-name, value].
  // The map may include:
  //  fTypes = <path to the types js file>
  //  fWrappers = <path to the wrappers js file>
  // Omitting a parameter, will simply not generate that file.
  // e.g.:
  //  If fTypes = "MyDir/MyTypes.js", the following file:
  //    "MyDir/MyTypes.js" will be automatically generated.
  //  Directory "MyDir" MUST exist.
  bool SetParams(const map<string, string>& paramValues);

  // Export types and wrappers into Javascript files.
  bool Export();

 public:
  static const string kLanguageName;

 private:
  // Map of pairs [RPC-BASE-TYPE, JS type]. Containing the mappings
  // of basic RPC types to JS types.
  // e.g. "bool" -> "rpc::Bool"
  //      "string" -> "rpc::String"
  //       ............
  static const map<string, string> kBaseTypesCorrespondence;
  // Builds a map of corresponding base types for Javascript.
  // This method is used just to fill in baseTypesCorrespondence_;
  static map<string, string> BuildBaseTypesCorrespondenceMap();
 public:
  // Returns a map of pairs [RPC-BASE-TYPE, JS Type]. Containing the mappings
  // of basic RPC types ("bool", "int", "float", ... ) to JS types.
  static const map<string, string>& BaseTypeCorrespondence();

 private:
  // Contains restricted keywords for Javascript: var, number, date, array, ..
  static const Keywords kRestrictedKeywords;
  // This method is used just to fill in keywords_;
  static Keywords BuildRestrictedKeywords();
 public:
  // Returns a set of keywords for Javascript.
  static const Keywords& RestrictedKeywords();

 private:
  // Returns a json array of Javascript base types corresponding to "type".
  // Used by auto-generated RpcRebuildObject(..) .
  // GenerateRebuildTypeInfoEnum returns just the enumeration
  // (comma separated values)
  // GenerateRebuildTypeInfoArray returns the enumeration enclosed in '[]'
  // e.g. type == int => ['number']
  //      type == date => ['date']
  //      type == array<array<float>> => ['array', 'array', 'number']
  //      type == Person => ['Person']
  string GenerateRebuildTypeInfoEnum(const PType& type);
  string GenerateRebuildTypeInfoArray(const PType& type);

 private:
  static const char paramTypes_[];
  static const char paramWrappers_[];

  // Contains a description of the parameters needed for the export.
  // The pairs are: paramName -> paramDescription
  static const map<string, string> paramsDescription_;

  // used to fill in paramsDescription_;
  static const map<string, string> BuildParamsDescription();

 public:
  static const map<string, string>& ParamsDescription();

 protected:
  // rpc-types.js : This file will include a definition of all custom-types.
  string fTypes_;

  // rpc-wrappers.js : This file will include a wrapper class for every service.
  // Permits easy & natural calls to remote functions.
  string fWrappers_;


  // Write a generic header for an auto-generated file.
  void ExportAutoGenFileHeader(ostream& out);

  bool ExportTypes(const char* types_js_file);
  bool ExportWrappers(const char* wrappers_js_file);

  // Checks all the types involved(customTypes,
  //                                functionReturns, functionArguments)
  // Rules:
  //  - only map<string, ... > is permitted.
  bool VerifyMapStringCorrectenss();
  // Checks the given type.
  bool VerifyMapStringCorrectness(const PFileInfo& fileinfo,
                                  const PType& type);

  DISALLOW_EVIL_CONSTRUCTORS(ExporterJS);
};
}

#endif  // __NET_RPC_PARSER_EXPORT_JAVASCRIPT_RPC_EXPORTER_JS_H__
