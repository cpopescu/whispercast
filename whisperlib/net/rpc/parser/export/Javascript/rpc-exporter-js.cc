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

#include <fstream>

#include "common/base/log.h"
#include "common/base/strutil.h"
#include "net/rpc/parser/version.h"
#include "net/rpc/parser/export/rpc-base-types.h"
#include "net/rpc/parser/export/Javascript/rpc-exporter-js.h"
#include "common/io/ioutil.h"

namespace rpc {

//static
const string ExporterJS::kLanguageName("Javascript");

// static
const map<string, string> ExporterJS::kBaseTypesCorrespondence(
    ExporterJS::BuildBaseTypesCorrespondenceMap());

// static
map<string, string> ExporterJS::BuildBaseTypesCorrespondenceMap() {
  map<string, string> btc;
  btc[RPC_VOID]   = "null";
  btc[RPC_BOOL]   = "boolean";
  btc[RPC_INT]    = "number";
  btc[RPC_BIGINT] = "number";
  btc[RPC_FLOAT]  = "number";
  btc[RPC_STRING] = "string";
  btc[RPC_ARRAY]  = "array";
  btc[RPC_MAP]    = "map";
  return btc;
}

// static
const map<string, string>& ExporterJS::BaseTypeCorrespondence() {
  return kBaseTypesCorrespondence;
}

//static
const Keywords ExporterJS::kRestrictedKeywords(
    ExporterJS::BuildRestrictedKeywords());

//static
Keywords ExporterJS::BuildRestrictedKeywords() {
  const char* res[] = {
      "break", "cas", "comment", "continue", "default", "delete", "do",
      "else", "export", "for", "function", "if", "import", "in", "label",
      "new", "return", "switch", "this", "typeof", "var", "void", "while",
      "with", "eval", "toString"
  };
  set<string> restricted;
  for ( unsigned i = 0; i < sizeof(res)/sizeof(res[0]); i++ ) {
    restricted.insert(res[i]);
  }
  return Keywords(kLanguageName, restricted, true);
}

//static
const Keywords& ExporterJS::RestrictedKeywords() {
  return kRestrictedKeywords;
}

const char ExporterJS::paramTypes_[] = "fTypes";
const char ExporterJS::paramWrappers_[] = "fWrappers";

string ExporterJS::GenerateRebuildTypeInfoEnum(const PType& type) {
  string tia;

  map<string, string>::const_iterator
      it = BaseTypeCorrespondence().find(type.name_);
  if ( it == BaseTypeCorrespondence().end() ) {
    // cannot find typename correspondence. It probably is a custom type.

    // We can accept some undefined custom types, as they can be included
    // from another file..

    tia = string("'") + type.name_ + "'";
  } else {
    tia = string("'") + it->second + "'";
  }

  if ( type.name_ == RPC_ARRAY ) {
    CHECK(type.subtype1_);
    tia += string(", ") + GenerateRebuildTypeInfoEnum(*type.subtype1_);
  } else if ( type.name_ == RPC_MAP ) {
    CHECK(type.subtype1_);
    CHECK(type.subtype2_);
    tia += string(", ") + GenerateRebuildTypeInfoEnum(*type.subtype2_);
  }

  return tia;
}

string ExporterJS::GenerateRebuildTypeInfoArray(const PType& type) {
  return string("[") + GenerateRebuildTypeInfoEnum(type) + "]";
}

// static
const map<string, string> ExporterJS::paramsDescription_(
    ExporterJS::BuildParamsDescription());

// static
const map<string, string> ExporterJS::BuildParamsDescription() {
  map<string, string> pd;
  pd[string(paramTypes_)]    = string("{.js} Custom types file.");
  pd[string(paramWrappers_)] = string("{.js} Service wrappers from "
                                      "client point of view. This "
                                      "are helpers, making the remote "
                                      "call easy & straight.");
  return pd;
}

// static
const map<string, string>& ExporterJS::ParamsDescription() {
  return paramsDescription_;
}

void ExporterJS::ExportAutoGenFileHeader(ostream& out) {
  out << "// " << APP_NAME_AND_VERSION << endl;
  out << "// " << endl;
  out << "// This is an auto generated file. DO NOT MODIFY !" << endl;
  out << "// The source files are: " << endl;
  for ( vector<string>::const_iterator it = inputFilenames_.begin();
        it != inputFilenames_.end(); ++it ) {
    const string& filename = *it;
    out << "//  - " <<  io::MakeAbsolutePath(filename.c_str()) << endl;
  }
  out << "//" << endl;
}

bool ExporterJS::ExportTypes(const char* types_js_file ) {
  ofstream out(types_js_file, ios::out | ios::trunc);
  if ( !out.is_open() ) {
    LOG_ERROR << "Cannot open output file: " << types_js_file;
    return false;
  }
  ExportAutoGenFileHeader(out);
  out << "" << endl;

  string verbatim = GetVerbatim();
  if ( !verbatim.empty() ) {
    out << "// Verbatim stuff included:" << endl
        << verbatim << endl;
  }

  for ( PCustomTypeArray::const_iterator it = customTypes_.begin();
        it != customTypes_.end(); ++it ) {
    const PCustomType& customType = *it;
    const PAttributeArray& attributes = customType.attributes_;
    out << "function " << customType.name_ << "(";
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ) {
      const PAttribute& attribute = *it;
      out << attribute.name_;
      ++it;
      if ( it != attributes.end() ) {
        out << ", ";
      }
    }
    out << ")" << endl;
    out << "{" << endl;
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ++it ) {
      const PAttribute& attribute = *it;
      out << "  this." << attribute.name_ << " = " << attribute.name_
          << ";" << endl;
    }
    out << "  " << endl;

    out << "  // load " << customType.name_
        << " from a pure javascript object" << endl;
    out << "  this.LoadFromObject = function(obj)" << endl;
    out << "  {" << endl;
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ++it ) {
      const PAttribute& attribute = *it;
      out << "    if(obj." << attribute.name_
          << " != undefined) this." << attribute.name_
          << " = RPCRebuildObject("
          << GenerateRebuildTypeInfoArray(attribute.type_)
          << ", obj." << attribute.name_ << ");" << endl;
    }
    out << "    //verify required attributes" << endl;
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ++it ) {
      const PAttribute& attribute = *it;
      if ( attribute.isOptional_ ) continue;
      out << "    if(this." << attribute.name_
          << " == undefined) throw \"Missing required attribute: "
          << attribute.name_ << "\";" << endl;
    }
    out << "  }" << endl;
    out << "" << endl;

    out << "  // used for JSON encoding" << endl;
    out << "  this.toString = function()" << endl;
    out << "  {" << endl;
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ++it ) {
      const PAttribute& attribute = *it;
      if ( attribute.isOptional_ ) {
        continue;
      }
      out << "    if(this." << attribute.name_
          << " == undefined) throw 'Missing required attribute: "
          << attribute.name_ << "';" << endl;
    }
    out << "    text = \"\";" << endl;
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ++it ) {
      const PAttribute& attribute = *it;
      if ( attribute.isOptional_ ) {
        out << "    if(this." << attribute.name_ << " != undefined)" << endl;
        out << "    {" << endl;
        out << "      text += (text == \"\" ? \"\" : \", \") + \"\\\""
            << attribute.name_ << "\\\" : \" + JSONEncodeValue(this."
            << attribute.name_ << ");" << endl;
        out << "    }" << endl;
        continue;
      }
      out << "    text += (text == \"\" ? \"\" : \", \") + \"\\\""
          << attribute.name_ << "\\\" : \" + JSONEncodeValue(this."
          << attribute.name_ << ");" << endl;
    }
    out << "    return \"{\" + text + \"}\";" << endl;
    out << "  }" << endl;
    out << "}" << endl;
    out << "" << endl;
  }

  out << "function RPCRebuildObject(arrStrTypes, obj)" << endl;
  out << "{" << endl;
  out << "  if(!InstanceOfArray(arrStrTypes))" << endl;
  out << "  {" << endl;
  out << "    throw \"Invalid arrStrTypes: not an array\";" << endl;
  out << "  }" << endl;
  out << "  if(arrStrTypes.length == 0)" << endl;
  out << "  {" << endl;
  out << "    throw \"Invalid arrStrTypes: empty array\";" << endl;
  out << "  }" << endl;
  out << "  switch(arrStrTypes[0])" << endl;
  out << "  {" << endl;
  out << "    case 'null'  :  "
      << " CHECK_EQ(obj, null);              return obj;" << endl;
  out << "    case 'string':  "
      << " CHECK_EQ(typeof(obj), 'string');  return obj;" << endl;
  out << "    case 'number':  "
      << " CHECK_EQ(typeof(obj), 'number');  return obj;" << endl;
  out << "    case 'boolean': "
      << " CHECK_EQ(typeof(obj), 'boolean'); return obj;" << endl;
  out << "    case 'date':    CHECK_EQ(typeof(obj), 'number');" << endl;
  out << "      var p = new Date();" << endl;
  out << "      p.setTime(obj);" << endl;
  out << "      return p;" << endl;
  out << "    case 'array':" << endl;
  out << "      var p = new Array(obj.length);" << endl;
  out << "      var remainingArrStrTypes = arrStrTypes.slice(1);" << endl;
  out << "      for(var i = 0; i < obj.length; i++)" << endl;
  out << "      {" << endl;
  out << "        p[i] = RPCRebuildObject(remainingArrStrTypes, obj[i]);"
      << endl;
  out << "      }" << endl;
  out << "      return p;" << endl;
  out << "    case 'map':" << endl;
  out << "      var p = new Object();" << endl;
  out << "      var remainingArrStrTypes = arrStrTypes.slice(1);" << endl;
  out << "      for(var attr in obj)" << endl;
  out << "      {" << endl;
  out << "        p[attr] = "
      << " RPCRebuildObject(remainingArrStrTypes, obj[attr]);" << endl;
  out << "      }" << endl;
  out << "      return p;" << endl;

  for ( PCustomTypeArray::const_iterator it = customTypes_.begin();
        it != customTypes_.end(); ++it ) {
    const PCustomType& customType = *it;
    out << "    case '" << customType.name_ << "':" << endl;
    out << "      var p = new " << customType.name_ << "();" << endl;
    out << "      p.LoadFromObject(obj);" << endl;
    out << "      return p;" << endl;
  }

  out << "    default:" << endl;
  out << "      LOG_ERROR << \"unknown type: \" + arrStrTypes[0] "
      << "+ \" in arrStrTypes: \" + arrStrTypes;" << endl;
  out << "      return obj;" << endl;
  out << "  };" << endl;
  out << "}" << endl;
  out << "" << endl;

  LOG_INFO << "Wrote file: " << types_js_file;
  return true;
}

bool ExporterJS::ExportWrappers(const char * wrappers_js_file) {
  ofstream out(wrappers_js_file, ios::out | ios::trunc);
  if ( !out.is_open() ) {
    LOG_ERROR << "Cannot open output file: [" << wrappers_js_file << "]";
    return false;
  }
  ExportAutoGenFileHeader(out);
  out << "" << endl;

  string verbatim = GetVerbatim();
  if ( !verbatim.empty() ) {
    out << "// Verbatim stuff included:" << endl
        << verbatim << endl;
  }

  for ( PServiceArray::const_iterator it = services_.begin();
        it != services_.end(); ++it ) {
    const PService& service = *it;
    const string& serviceName = service.name_;

    out << "function RPCServiceWrapper" << serviceName
        << "(rpc_connection, service_name)" << endl;
    out << "{" << endl;
    out << "  if(rpc_connection == undefined)" << endl;
    out << "  {" << endl;
    out << "    LOG_ERROR(\"Bad rpc_connection: \" + rpc_connection);" << endl;
    out << "    throw \"Bad rpc_connection: \" + rpc_connection;" << endl;
    out << "  }" << endl;
    out << "  this.rpc_connection_ = rpc_connection;" << endl;
    out << "  this.service_name_ = (service_name == undefined ?"
           "'" << serviceName << "' : service_name);" << endl;
    out << "  " << endl;
    const PFunctionArray& functions = service.functions_;
    for ( PFunctionArray::const_iterator it = functions.begin();
          it != functions.end(); ++it ) {
      const PFunction& function = *it;
      const string& functionName = function.name_;
      const PType& functionReturn = function.output_;
      const PParamArray& args = function.input_;

      out << "  //" << functionReturn << " " << functionName << "(";

      for ( PParamArray::const_iterator it = args.begin();
            it != args.end(); ) {
        const PParam& arg = *it;
        out << arg.type_ << " " << arg.name_;
        ++it;
        if ( it != args.end() ) {
          out << ", ";
        }
      }
      out << ");" << endl;
      out << "  this." << functionName << " = function(";
      for ( PParamArray::const_iterator it = args.begin();
            it != args.end(); ++it ) {
        const PParam& arg = *it;
        out << arg.name_ << ", ";
      }
      out << "handleResultCallback, userData)" << endl;
      out << "  {" << endl;
      for ( PParamArray::const_iterator it = args.begin();
            it != args.end(); ++it ) {
        const PParam& arg = *it;
        out << "    CHECK(" << arg.name_
            << " != undefined, \"auto-wrappers.js: '" << functionName
            << "': undefined argument '" << arg.name_ << "'\");" << endl;
      }
      out << "    CHECK(typeof(handleResultCallback) == 'function', "
          << "\"auto-wrappers.js: '" << functionName
          << "': 'handleResultCallback' must be a function\");" << endl;
      out << "    var params = JSONEncodeValue([";
      for ( PParamArray::const_iterator it = args.begin();
            it != args.end(); ) {
        const PParam& arg = *it;
        out << arg.name_;
        ++it;
        if ( it != args.end() ) {
          out << ", ";
        }
      }
      out << "]);" << endl;
      out << "    this.rpc_connection_.Call(this.service_name_"
             ", \"" << functionName << "\", params," << endl;
      out << "      function(callStatus, result)" << endl;
      out << "      {" << endl;
      out << "        if(callStatus.success_)" << endl;
      out << "          handleResultCallback(callStatus, RPCRebuildObject("
          << GenerateRebuildTypeInfoArray(functionReturn)
          << ", result));" << endl;
      out << "        else" << endl;
      out << "          handleResultCallback(callStatus, undefined);" << endl;
      out << "      }, userData);" << endl;
      out << "  }" << endl;
      out << "  " << endl;
    }
    out << "}" << endl;
    out << "" << endl;
  }

  LOG_INFO << "Wrote file: " << wrappers_js_file;
  return true;
}

bool ExporterJS::VerifyMapStringCorrectenss() {
  bool success = true;
  for ( PCustomTypeArray::const_iterator it = customTypes_.begin();
        it != customTypes_.end(); ++it ) {
    const PCustomType& customType = *it;
    const PAttributeArray& attributes = customType.attributes_;
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ++it ) {
      const PAttribute& attribute = *it;
      success = (VerifyMapStringCorrectness(attribute, attribute.type_) &&
                 success);
    }
  }
  for ( PServiceArray::const_iterator it = services_.begin();
        it != services_.end(); ++it ) {
    const PService& service = *it;
    const PFunctionArray& functions = service.functions_;
    for ( PFunctionArray::const_iterator it = functions.begin();
          it != functions.end(); ++it ) {
      const PFunction& function = *it;
      const PType& functionReturn = function.output_;
      success = VerifyMapStringCorrectness(function, functionReturn) && success;
      const PParamArray& args = function.input_;
      for ( PParamArray::const_iterator it = args.begin();
            it != args.end(); ++it ) {
        const PParam& arg = *it;
        success = VerifyMapStringCorrectness(function, arg.type_) && success;
      }
    }
  }
  return success;
}

bool ExporterJS::VerifyMapStringCorrectness(const PFileInfo& fileinfo,
                                            const PType& type) {
  if ( type.name_ == RPC_ARRAY ) {
    CHECK(type.subtype1_);
    return VerifyMapStringCorrectness(fileinfo, *type.subtype1_);
  } else if ( type.name_ == RPC_MAP ) {
    CHECK(type.subtype1_);
    CHECK(type.subtype2_);
    if ( type.subtype1_->name_ == RPC_STRING ) {
    } else if ( type.subtype1_->name_ == RPC_INT ||
                type.subtype1_->name_ == RPC_BIGINT ||
                type.subtype1_->name_ == RPC_FLOAT ) {
      LOG_WARNING
          << "Javascript does not support map<"
          << type.subtype1_->name_
          << ",..> natively. To support this type, we will transfer "
          << "it as map<string,...>.";
    } else {
      LOG_ERROR << "Javascript supports only map<string,...> native "
                << "and map<int,...>, map<bigint,...>, map<float,...>, "
                << "map<double,...> as part of our private convention.";
      return false;
    }
    return VerifyMapStringCorrectness(fileinfo, *type.subtype2_);
  }
  return true;
}


ExporterJS::ExporterJS(const PCustomTypeArray& customTypes,
                       const PServiceArray& services,
                       const PVerbatimArray& verbatim,
                       const vector<string>& inputFilenames)
    : Exporter(kLanguageName, customTypes, services, verbatim, inputFilenames),
      fTypes_(),
      fWrappers_() {
}

ExporterJS::~ExporterJS() {
}

const Keywords& ExporterJS::GetRestrictedKeywords() const {
  return ExporterJS::kRestrictedKeywords;
}

bool ExporterJS::SetParams(const map<string, string>& paramValues) {
  // map of [paramName, address of local attribute] e.g.
  // ["fClientType", &fClientType_]
  typedef pair<string, string*> ParamNameValue;

  // map of [paramName in lower case, [original paramName, address
  // of local attribute]] e.g. ["fclienttype", ["fClientType", &fClientType_]]
  typedef map<string, ParamNameValue> InnerParamMap;

  InnerParamMap innerParams;

#define APPEND_PARAM(paramOriginalName, pValue) {                       \
    string pOriginal(paramOriginalName);                                \
    string pLowerCase(paramOriginalName);                               \
    strutil::StrToLower(pLowerCase);                                    \
    InnerParamMap::value_type                                           \
        innerParam(pLowerCase, ParamNameValue(pOriginal, pValue));      \
    pair<InnerParamMap::iterator, bool>                                 \
        result = innerParams.insert(innerParam);                        \
    CHECK(result.second);                                               \
  }

  APPEND_PARAM(paramTypes_, &fTypes_);
  APPEND_PARAM(paramWrappers_, &fWrappers_);
#undef APPEND_PARAM

  for ( map<string, string>::const_iterator it = paramValues.begin();
        it != paramValues.end(); ++it ) {
    const string& cproperty = it->first;
    const string& value = it->second;

    string property(cproperty);
    strutil::StrToLower(property);

    InnerParamMap::iterator innerIt = innerParams.find(property);
    if ( innerIt == innerParams.end() ) {
      LOG_ERROR << "Unknown parameter: \"" << cproperty
                << "\" = " << value;
      continue;
    }

    ParamNameValue& paramNameValue = innerIt->second;
    *paramNameValue.second = io::MakeAbsolutePath(value.c_str());

    LOG_INFO << "Set parameter: " << paramNameValue.first << " = " << value;
  }

  return true;
}

bool ExporterJS::Export() {
  return ( VerifyMapStringCorrectenss() &&
           (fTypes_.empty() || ExportTypes(fTypes_.c_str())) &&
           (fWrappers_.empty() || ExportWrappers(fWrappers_.c_str())) );
}
}
