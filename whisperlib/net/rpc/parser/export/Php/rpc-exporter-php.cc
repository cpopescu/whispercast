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
#include "net/rpc/parser/export/Php/rpc-exporter-php.h"
#include "common/io/ioutil.h"

namespace rpc {

//static
const string ExporterPHP::kLanguageName("PHP");

// static
const map<string, string> ExporterPHP::kBaseTypesCorrespondence(
    ExporterPHP::BuildBaseTypesCorrespondenceMap());

// static
map<string, string> ExporterPHP::BuildBaseTypesCorrespondenceMap() {
  map<string, string> btc;
  btc[RPC_VOID]   = "null";
  btc[RPC_BOOL]   = "boolean";
  btc[RPC_INT]    = "integer";
  btc[RPC_BIGINT] = "integer";
  btc[RPC_FLOAT]  = "float";
  btc[RPC_STRING] = "string";
  btc[RPC_ARRAY]  = "array";
  btc[RPC_MAP]    = "map";
  return btc;
}

// static
const map<string, string>& ExporterPHP::BaseTypeCorrespondence() {
  return kBaseTypesCorrespondence;
}

//static
const Keywords ExporterPHP::kRestrictedKeywords(
    ExporterPHP::BuildRestrictedKeywords());

//static
Keywords ExporterPHP::BuildRestrictedKeywords() {
  const char* res[] = {
      "abstract", "and", "array", "as", "break",
      "case", "catch", "cfunction", "class", "clone",
      "const", "continue", "declare", "default", "do",
      "else", "elseif", "enddeclare", "endfor", "endforeach",
      "endif", "endswitch", "endwhile", "extends", "final",
      "for", "foreach", "function", "global", "goto",
      "if", "implements", "interface", "instanceof",
      "namespace", "new", "old_function", "or", "private",
      "protected", "public", "static", "switch", "throw",
      "try", "use", "var", "while", "xor",
      "__CLASS__", "__DIR__", "__FILE__", "__FUNCTION__", "__METHOD__",
      "__NAMESPACE__",
      "die", "echo", "empty", "exit", "eval",
      "include", "include_once", "isset", "list", "require",
      "require_once", "return", "print", "unset"
  };
  set<string> restricted;
  for ( unsigned i = 0; i < sizeof(res)/sizeof(res[0]); i++ ) {
    restricted.insert(res[i]);
  }
  return Keywords(kLanguageName, restricted, true);
}

//static
const Keywords& ExporterPHP::RestrictedKeywords() {
  return kRestrictedKeywords;
}

const char ExporterPHP::paramTypes_[] = "fTypes";
const char ExporterPHP::paramWrappers_[] = "fWrappers";

string ExporterPHP::GenerateRebuildTypeInfoEnum(const PType& type) {
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

string ExporterPHP::GenerateRebuildTypeInfoArray(const PType& type) {
  return string("array(") + GenerateRebuildTypeInfoEnum(type) + ")";
}

// static
const map<string, string> ExporterPHP::paramsDescription_(
    ExporterPHP::BuildParamsDescription());

// static
const map<string, string> ExporterPHP::BuildParamsDescription() {
  map<string, string> pd;
  pd[string(paramTypes_)]    = string("{.php} Custom types file.");
  pd[string(paramWrappers_)] = string("{.php} Service wrappers from "
                                      "client point of view. This "
                                      "are helpers, making the remote "
                                      "call easy & straight.");
  return pd;
}

// static
const map<string, string>& ExporterPHP::ParamsDescription() {
  return paramsDescription_;
}

void ExporterPHP::ExportAutoGenFileHeader(ostream& out) {
  out << "<?php" << endl;
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

bool ExporterPHP::ExportTypes(const char* types_php_file ) {
  ofstream out(types_php_file, ios::out | ios::trunc);
  if ( !out.is_open() ) {
    LOG_ERROR << "Cannot open output file: " << types_php_file;
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
    out << "class " << customType.name_ << " {" << endl;
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ++it) {
      const PAttribute& attribute = *it;
      out << "  public $" << attribute.name_ << ";" << endl;
    }
    out << "  " << endl;

    out << "  function __construct(";
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ) {
      const PAttribute& attribute = *it;
      out << "$" << attribute.name_ << " = NULL";
      ++it;
      if ( it != attributes.end() ) {
        out << ", ";
      }
    }
    out << ") {" << endl;
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ++it ) {
      const PAttribute& attribute = *it;
      out << "    $this->" << attribute.name_ << " = $" << attribute.name_
          << ";" << endl;
    }
    out << "  }" << endl;
    out << "  " << endl;

    out << "  // load " << customType.name_
        << " from a pure object" << endl;
    out << "  public function LoadFromObject($obj) {" << endl;
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ++it ) {
      const PAttribute& attribute = *it;
      out << "    if($obj->" << attribute.name_ << " !== NULL)"
             " $this->" << attribute.name_ << " = RpcRebuildObject("
          << GenerateRebuildTypeInfoArray(attribute.type_)
          << ", $obj->" << attribute.name_ << ");" << endl;
    }
    out << "    //verify required attributes" << endl;
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ++it ) {
      const PAttribute& attribute = *it;
      if ( attribute.isOptional_ ) continue;
      out << "    if($this->" << attribute.name_
          << " === NULL) throw new Exception(\"Missing required attribute: "
          << attribute.name_ << "\");" << endl;
    }
    out << "  }" << endl;
    out << "" << endl;

    out << "  // used for JSON encoding" << endl;
    out << "  public function __toString() {" << endl;
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ++it ) {
      const PAttribute& attribute = *it;
      if ( attribute.isOptional_ ) {
        continue;
      }
      out << "    if($this->" << attribute.name_
          << " === NULL) throw new Exception(\"Missing required attribute: "
          << attribute.name_ << "\");" << endl;
    }
    out << "    $text = \"\";" << endl;
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ++it ) {
      const PAttribute& attribute = *it;
      if ( attribute.isOptional_ ) {
        out << "    if($this->" << attribute.name_ << " !== NULL)" << endl;
        out << "    {" << endl;
        out << "      $text .= ($text === \"\" ? \"\" : \", \") . \"\\\""
            << attribute.name_ << "\\\" : \" . JsonEncodeValue($this->"
            << attribute.name_ << ");" << endl;
        out << "    }" << endl;
        continue;
      }
      out << "    $text .= ($text === \"\" ? \"\" : \", \") . \"\\\""
          << attribute.name_ << "\\\" : \" . JsonEncodeValue($this->"
          << attribute.name_ << ");" << endl;
    }
    out << "    return \"{\" . $text . \"}\";" << endl;
    out << "  }" << endl;
    out << "}" << endl;
    out << "" << endl;
  }
  out << "" << endl;
  out << "?>" << endl;
  out << "" << endl;

  LOG_INFO << "Wrote file: " << types_php_file;
  return true;
}

bool ExporterPHP::ExportWrappers(const char * wrappers_php_file) {
  ofstream out(wrappers_php_file, ios::out | ios::trunc);
  if ( !out.is_open() ) {
    LOG_ERROR << "Cannot open output file: [" << wrappers_php_file << "]";
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

    out << "class RpcServiceWrapper" << serviceName << " {" << endl;
    out << "  " << endl;

    out << "  private $rpc_connection_;" << endl;
    out << "  private $service_name_;" << endl;
    out << "  " << endl;

    out << "  function __construct(RpcConnection $rpc_connection"
                                              ", $service_name) {" << endl;
    out << "    CHECK($rpc_connection !== NULL);" << endl;
    out << "    CHECK($service_name !== NULL);" << endl;
    out << "    $this->rpc_connection_ = $rpc_connection;" << endl;
    out << "    $this->service_name_ = $service_name;" << endl;
    out << "  }" << endl;
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
      out << "  public function " << functionName << "(";
      for ( PParamArray::const_iterator it = args.begin();
            it != args.end(); ) {
        const PParam& arg = *it;
        out << "$" << arg.name_;
        ++it;
        if ( it != args.end() ) {
          out << ", ";
        }
      }
      out << ") {" << endl;
      for ( PParamArray::const_iterator it = args.begin();
            it != args.end(); ++it ) {
        const PParam& arg = *it;
        out << "    CHECK($" << arg.name_ << " !== NULL);" << endl;
      }
      out << "    $params = JsonEncodeValue(array(";
      for ( PParamArray::const_iterator it = args.begin();
            it != args.end(); ) {
        const PParam& arg = *it;
        out << "$" << arg.name_;
        ++it;
        if ( it != args.end() ) {
          out << ", ";
        }
      }
      out << "));" << endl;
      out << "      $call_status = $this->rpc_connection_->Call("
             "$this->service_name_, \"" << functionName << "\", $params);"
          << endl;
      out << "      if(!$call_status->success_) {" << endl;
      out << "        return $call_status; " << endl;
      out << "      }" << endl;
      out << "      $call_status->result_ = RpcRebuildObject("
          << GenerateRebuildTypeInfoArray(functionReturn)
          << ", $call_status->result_array_);" << endl;
      out << "      return $call_status;" << endl;
      out << "  }" << endl;
      out << "  " << endl;
    }
    out << "}" << endl;
    out << "" << endl;
  }
  out << "?>" << endl;
  out << "" << endl;

  LOG_INFO << "Wrote file: " << wrappers_php_file;
  return true;
}

bool ExporterPHP::VerifyMapStringCorrectenss() {
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

bool ExporterPHP::VerifyMapStringCorrectness(const PFileInfo& fileinfo,
                                             const PType& type) {
  if ( type.name_ == RPC_ARRAY ) {
    CHECK(type.subtype1_);
    return VerifyMapStringCorrectness(fileinfo, *type.subtype1_);
  } else if ( type.name_ == RPC_MAP ) {
    CHECK(type.subtype1_);
    CHECK(type.subtype2_);
    if ( type.subtype1_->name_ == RPC_STRING ||
         type.subtype1_->name_ == RPC_INT ||
         type.subtype1_->name_ == RPC_BIGINT ) {
    } else if ( type.subtype1_->name_ == RPC_FLOAT ) {
      LOG_WARNING << "Php does not support map<"
                  << type.subtype1_->name_
                  << ",..> natively. To support this type, we will transfer "
                  << "it as map<string,...>.";
    } else {
      LOG_ERROR << "Php has native support only for: map<string,...>, "
                   "map<int,...>, map<bigint,...>. Extended support: "
                   "map<float,...> is transferred as map<string,...> as "
                   "part of our private convention.";
      return false;
    }
    return VerifyMapStringCorrectness(fileinfo, *type.subtype2_);
  }
  return true;
}


ExporterPHP::ExporterPHP(const PCustomTypeArray& customTypes,
                         const PServiceArray& services,
                         const PVerbatimArray& verbatim,
                         const vector<string>& inputFilenames)
    : Exporter(kLanguageName, customTypes, services, verbatim, inputFilenames),
      fTypes_(),
      fWrappers_() {
}

ExporterPHP::~ExporterPHP() {
}

const Keywords& ExporterPHP::GetRestrictedKeywords() const {
  return ExporterPHP::kRestrictedKeywords;
}

bool ExporterPHP::SetParams(const map<string, string>& paramValues) {
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

bool ExporterPHP::Export() {
  return ( VerifyMapStringCorrectenss() &&
           (fTypes_.empty() || ExportTypes(fTypes_.c_str())) &&
           (fWrappers_.empty() || ExportWrappers(fWrappers_.c_str())) );
}
}
