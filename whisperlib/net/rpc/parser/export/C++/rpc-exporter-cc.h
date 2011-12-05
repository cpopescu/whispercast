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

#ifndef __NET_RPC_PARSER_EXPORT_CPP_RPC_EXPORTER_CC_H__
#define __NET_RPC_PARSER_EXPORT_RPC_BASE_TYPES_H__

#include <map>
#include <string>
#include <vector>

#include <whisperlib/net/rpc/parser/export/rpc-exporter.h>

namespace rpc {

class ExporterCC : public Exporter {
 public:
  ExporterCC(const PCustomTypeArray& customTypes,
             const PServiceArray& services,
             const PVerbatimArray& verbatim,
             const vector<string>& inputFilenames);
  virtual ~ExporterCC();

  //////////////////////////////////////////////////////////////////////
  //
  //                    Exporter interface methods

  const Keywords& GetRestrictedKeywords() const;

  // Pairs of [property-name, value]. These are exporter specific parameters.
  // e.g. for the C++ exporter the map should include:
  //        fclientTypes = <path to client rpc-types(.h .cc)>
  //        fclientWrappers = <path to client rpc-wrappers(.h .cc)>
  //        fserverTypes = <path to server rpc-types(.h .cc)>
  //        fserverMethods = <path to server rpc-services-methods.h>
  //        fserverImpl = <path to server rpc-services-impl(.h .cc)>
  //        includePathCut = If specified, we cut this string from
  //                         the beginning of the include paths we generate.
  //        includePathPaste = If specified, we paste this string at
  //                           the beginning of the include paths we generate.
  //
  //
  //  If fclientTypes = "MyDir/MyTypes", the following files:
  //    "MyDir/MyTypes.h" and "MyDir/MyTypes.cc" will
  //    be automatically generated.
  //  Directory "MyDir" MUST exist.
  bool SetParams(const map<string, string>& paramValues);

  // Export types and services into C++ files.
  bool Export();

 public:
  static const string kLanguageName;

 private:
  static const map<string, string> kBaseTypesCorrespondence;
  static const map<string, string> kBaseTypesArgCorrespondence;
  // Builds a map of corresponding base types for C++.
  // This method is used just to fill in baseTypesCorrespondence_;
  static const map<string, string> BuildBaseTypesCorrespondenceMap();
  // Builds a map of corresponding base argument types for C++.
  // This method is used just to fill in baseTypesArgCorrespondence_;
  static const map<string, string> BuildBaseTypesArgCorrespondenceMap();
 public:
   // Returns a map of pairs [RPC-BASE-TYPE, C++Type]. Containing the mappings
   // of basic RPC types ("BOOL", "INT32", "FLOAT", ... ) to C++ types.
  static const map<string, string>& BaseTypeCorrespondence();

 private:
  // Contains restricted keywords for C++: bool, break, case, class ..
  static const Keywords kRestrictedKeywords;
  // This method is used just to fill in restrictedKeywords_;
  static Keywords BuildRestrictedKeywords();
 public:
  // Returns a set of restricted keywords for Javascript.
  static const Keywords& RestrictedKeywords();

 private:
  static const string TranslateAttribute(const PType& type);
  static const string TranslateType(const PType& type);

 private:
  static const char paramClientTypes_[];
  static const char paramClientWrappers_[];
  static const char paramServerTypes_[];
  static const char paramServerInvokers_[];
  static const char paramIncludePathCut_[];
  static const char paramIncludePathPaste_[];

  // Contains a description of the parameters needed for the export.
  // The pairs are: paramName -> paramDescription
  static const map<string, string> paramsDescription_;

  // used to fill in paramsDescription_;
  static const map<string, string> BuildParamsDescription();

 public:
  static const map<string, string>& ParamsDescription();

 protected:
  // rpc-types(.h .cc) All custom-types, with encode and decode methods.
  // This file is identical for client & server. Both of them need to
  // use exactly the same types.
  // This filename will be "#include"d in other client generated files.
  string fClientTypes_;

  // rpc-wrappers(.h .cc) A wrapper class for every service. Permits easy &
  // natural calls to remote functions.
  string fClientWrappers_;

  // rpc-types(.h .cc) All custom-types, with encode and decode methods.
  // This file is identical for client & server. Both of them need to
  // use exactly the same types.
  // This filename will be "#include"d in other server generated files.
  string fServerTypes_;

  // rpc-invokers(.h .cc) A complete ServiceInvoker class for every
  // service, capable of: decode call parameters, execute call, encode result.
  string fServerInvokers_;

  // If specified, we cut this string from
  // the beginning of the include paths we generate.
  string includePathCut_;

  // If specified, we paste this string at
  // the beginning of the include paths we generate.
  string includePathPaste_;

  // Helper for generating an include line from a path (taking in account
  // the path cut / paste)
  string IncludeLine(const string include_path) const;

  // Write a generic header for an auto-generated file.
  void ExportAutoGenFileHeader(ostream& out);

  bool ExportTypesH(const char* clientTypes_h_file);
  bool ExportTypesCC(const char* include_h_file,
                     const char* clientTypes_cc_file);

  bool ExportClientWrappersH(const char* clientWrappers_h_file);
  bool ExportClientWrappersCC(const char* clientWrappers_cc_file);

  bool ExportServerInvokersH(const char* serverInvokers_h_file);
  bool ExportServerInvokersCC(const char* serverInvokers_cc_file);

 private:

  DISALLOW_EVIL_CONSTRUCTORS(ExporterCC);
};
}
#endif   // __NET_RPC_PARSER_EXPORT_CPP_RPC_EXPORTER_CC_H__
