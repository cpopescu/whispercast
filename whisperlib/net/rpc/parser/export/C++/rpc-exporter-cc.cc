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

#include <set>
#include <fstream>   // ofstream, ios::out, ios::trunc
#include <algorithm>

#include "net/rpc/parser/export/rpc-base-types.h"
#include "net/rpc/parser/export/C++/rpc-exporter-cc.h"
#include "net/rpc/parser/version.h"
#include "common/base/types.h"
#include "common/base/log.h"
#include "common/base/strutil.h"
#include "common/io/ioutil.h"

namespace {
char FileIDCharsTransformer(const char & c) {
  if ( c >= 'A' && c <= 'Z' ) return c;
  if ( c >= 'a' && c <= 'z' ) return ::toupper(c);
  if ( c >= '0' && c <= '9' ) return c;
  return '_';
}

std::string MakeFileIdentifier(const char* path) {
  string id = io::MakeAbsolutePath(path);
  std::transform(id.begin(), id.end(), id.begin(), FileIDCharsTransformer);
  return id;
}
}

namespace rpc {

//static
const string ExporterCC::kLanguageName("cc");

//static
const map<string, string>
ExporterCC::kBaseTypesCorrespondence
(ExporterCC::BuildBaseTypesCorrespondence());

//static
const set<string>
ExporterCC::kSimpleBaseTypes
(ExporterCC::BuildSimpleBaseTypes());

const map<string, string> ExporterCC::BuildBaseTypesCorrespondence() {
  map<string, string> btc;
  btc[string(RPC_VOID)]   = string("rpc::Void");
  btc[string(RPC_BOOL)]   = string("bool");
  btc[string(RPC_INT)]    = string("int32");
  btc[string(RPC_BIGINT)] = string("int64");
  btc[string(RPC_FLOAT)]  = string("double");
  btc[string(RPC_STRING)] = string("string");
  btc[string(RPC_ARRAY)]  = string("vector");
  btc[string(RPC_MAP)]    = string("map");
  return btc;
}
const set<string> ExporterCC::BuildSimpleBaseTypes() {
  set<string> s;
  s.insert(string(RPC_BOOL));
  s.insert(string(RPC_INT));
  s.insert(string(RPC_BIGINT));
  s.insert(string(RPC_FLOAT));
  return s;
}

//static
const Keywords ExporterCC::kRestrictedKeywords(
    ExporterCC::BuildRestrictedKeywords());

//static
Keywords ExporterCC::BuildRestrictedKeywords() {
  const char* res[] = {
      // generic keywords
      "var", "function", "asm", "auto", "bool", "break", "case", "catch",
      "char", "class", "const", "const_cast", "continue", "default", "delete",
      "do", "double", "dynamic_cast", "else", "enum", "explicit", "export",
      "extern", "false", "float", "for", "friend", "goto", "if", "inline",
      "int", "long", "mutable", "namespace", "new", "operator", "private",
      "protected", "public", "register", "reinterpret_cast", "return", "short",
      "signed", "sizeof", "static", "static_cast", "struct", "switch",
      "template", "this", "throw", "true", "try", "typedef", "typeid",
      "typename", "union", "unsigned", "using", "virtual", "void", "volatile",
      "wchar_t", "while",
      // types defined by rpc-lib
      "Bool", "Int", "BigInt", "Float", "Double", "String", "Void",
      "Map", "Array"
  };
  set<string> restricted;
  for ( unsigned i = 0; i < sizeof(res)/sizeof(res[0]); i++ ) {
    restricted.insert(res[i]);
  }
  return Keywords(kLanguageName, restricted, true);
}

//static
const Keywords& ExporterCC::RestrictedKeywords() {
  return kRestrictedKeywords;
}

const char ExporterCC::paramClientTypes_[] = "fClientTypes";
const char ExporterCC::paramClientWrappers_[] = "fClientWrappers";
const char ExporterCC::paramServerTypes_[] = "fServerTypes";
const char ExporterCC::paramServerInvokers_[] = "fServerInvokers";
const char ExporterCC::paramIncludePathCut_[] = "includePathCut";
const char ExporterCC::paramIncludePathPaste_[] = "includePathPaste";

string ExporterCC::IncludeLine(const string include_path) const {
  string inc(include_path);
  if ( !includePathCut_.empty() &&
       strutil::StrStartsWith(inc, includePathCut_) ) {
    LOG_INFO << " inc1 : " << inc;
    inc = inc.substr(includePathCut_.size());
    LOG_INFO << " inc2 : " << inc;
  }
  if ( !includePathPaste_.empty() ) {
    LOG_INFO << " includePathPaste_: " << includePathPaste_;
    LOG_INFO << " inc: " << inc;
    LOG_INFO << " inc3 : " << "#include <" + includePathPaste_ + inc + ">";
    return "#include <" + includePathPaste_ + inc + ">";
  } else {
    LOG_INFO << " inc4 : " << "#include \"" + inc + "\"";
    return "#include \"" + inc + "\"";
  }
}
const string ExporterCC::TranslateAttribute(const PType& type) {
  if ( type.name_ == RPC_STRING ) {
    return "StringAttribute";
  } else if ( type.name_ == RPC_ARRAY ) {
    return strutil::StringPrintf("VectorAttribute< %s >",
                                 TranslateType(type).c_str());
  } else if ( type.name_ == RPC_MAP ) {
    return strutil::StringPrintf("MapAttribute< %s >",
                                 TranslateType(type).c_str());
  } else {
    return strutil::StringPrintf("Attribute< %s >",
                                 TranslateType(type).c_str());
  }
}
const string ExporterCC::TranslateType(const PType& type) {
  string translatedType;

  map<string, string>::const_iterator it =
      kBaseTypesCorrespondence.find(type.name_);
  if ( it == kBaseTypesCorrespondence.end() ) {
    // cannot find typename correspondence. It probably is a custom type.

    // We can accept some undefined custom types, as they can be included
    // from another file..

    translatedType = type.name_;
  } else {
    translatedType = it->second;
  }

  if ( type.name_ == RPC_ARRAY ) {
    CHECK_NOT_NULL(type.subtype1_);
    translatedType += (string("< ") +
                       TranslateType(*type.subtype1_) + string(" >"));
  } else if ( type.name_ == RPC_MAP ) {
    CHECK_NOT_NULL(type.subtype1_);
    CHECK_NOT_NULL(type.subtype2_);
    translatedType += (string("< ") +
                       TranslateType(*type.subtype1_) + string(", ") +
                       TranslateType(*type.subtype2_) + string(" >"));
  }
  return translatedType;
}
bool ExporterCC::IsSimpleType(const PType& type) {
  return kSimpleBaseTypes.find(type.name_) != kSimpleBaseTypes.end();
}

const map<string, string> ExporterCC::paramsDescription_(
    ExporterCC::BuildParamsDescription());

const map<string, string> ExporterCC::BuildParamsDescription() {
  map<string, string> pd;
  pd[string(paramClientTypes_)]    =
      string("{.h .cc} Custom types file for client.");
  pd[string(paramClientWrappers_)] = string(
      "{.h .cc} Service wrappers from client point of view: with encode, "
      "send request, wait, decode result. This is are helpers, making "
      "the remote call easy& straight, but using them is not mandatory.");
  pd[string(paramServerTypes_)]    =
      string("{.h .cc} Custom types file for server.");
  pd[paramServerInvokers_] = string(
      "{.h .cc} Service invokers from server point of view. Each invoker "
      "contains virtual declarations of service function. You'll have "
      "to extend the invokers & implement service functions in your "
      "own files");
  pd[paramIncludePathCut_] = string(
      "If specified, we cut this string from "
      "the beginning of the include paths we generate.");
  pd[paramIncludePathPaste_] = string(
      "If specified, we paste this string at "
      "the beginning of the include paths we generate.");

  return pd;
}

const map<string, string>& ExporterCC::ParamsDescription() {
  return paramsDescription_;
}

void ExporterCC::ExportAutoGenFileHeader(ostream& out) {
  out << "// " << APP_NAME_AND_VERSION << endl;
  out << "// " << endl;
  out << "// This is an auto generated file. DO NOT MODIFY !" << endl;
  out << "// The source files are: " << endl;
  for ( vector<string>::const_iterator it = inputFilenames_.begin();
        it != inputFilenames_.end(); ++it) {
    const string& filename = *it;
    out << "//  - " <<  filename.c_str() << endl;
  }
  out << "//" << endl;
}

bool ExporterCC::ExportTypesH(const char* types_h_file) {
  ofstream out(types_h_file, ios::out | ios::trunc);
  if ( !out.is_open() ) {
    LOG_ERROR << "Cannot open output file: " << types_h_file
              << ", err: " << GetLastSystemErrorDescription();
    return false;
  }
  ExportAutoGenFileHeader(out);

  string strFileIdentifier = MakeFileIdentifier(types_h_file);
  out << "#ifndef " << strFileIdentifier << endl;
  out << "#define " << strFileIdentifier << endl;

  // include all needed files
  //
  out << "" << endl;
  out << "#include <whisperlib/common/base/types.h>" << endl;
  out << "#include <whisperlib/net/rpc/lib/types/rpc_all_types.h>" << endl;
  out << "" << endl;

  string verbatim = GetVerbatim();
  if ( !verbatim.empty() ) {
    out << "// Verbatim stuff included:" << endl
        << verbatim << endl;
  }
  for ( PCustomTypeArray::const_iterator it = customTypes_.begin();
        it != customTypes_.end(); ++it ) {
    const PCustomType& customType = *it;
    out << "class " << customType.name_ << " : public rpc::Custom" << endl;
    out << "{" << endl;
    out << "public:" << endl;

    const PAttributeArray& attributes = customType.attributes_;
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ++it ) {
      const PAttribute& attribute = *it;
      out << "  " << TranslateAttribute(attribute.type_)
          << "  " << attribute.name_ << ";" << endl;
    }
    out << "  " << endl;

    out << "public:" << endl;
    out << "  static inline const char*  name() { return \""
        << customType.name_ << "\"; }" << endl;
    out << endl;

    out << "public:" << endl;
    out << "  " << customType.name_ << "();" << endl;
    if ( !attributes.empty() ) {
      // generate constructor(args) only if args are not empty,
      // otherwise we already have constructor()
      out << "  " << customType.name_ << "(";
      for ( PAttributeArray::const_iterator it = attributes.begin();
            it != attributes.end(); ) {
        const PAttribute& attribute = *it;
        out << TranslateType(attribute.type_) << " _" << attribute.name_;
        ++it;
        if ( it != attributes.end() ) {
          out << ", ";
        }
      }
      out << ");" << endl;
    }
    out << "  " << customType.name_
        << "(const " << customType.name_ << " &);" << endl;
    out << "  virtual ~" << customType.name_ << "();" << endl;
    out << "  " << endl;

    out << "  void Fill(";
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ) {
      const PAttribute& attribute = *it;
      out << TranslateType(attribute.type_) << " _" << attribute.name_;
      ++it;
      if ( it != attributes.end() ) {
        out << ", ";
      }
    }
    out << ");" << endl;

    out << "  void FillRef(";
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ) {
      const PAttribute& attribute = *it;
      out << "const " << TranslateType(attribute.type_) << "& _"
          << attribute.name_;
      ++it;
      if ( it != attributes.end() ) {
        out << ", ";
      }
    }
    out << ");" << endl;
    out << "  " << endl;

    out << "  static string GetHTMLFormFunction();" << endl;
    out << "  " << endl;

    out << "public:" << endl;
    out << "  ////////////////////////////////////////////////////" << endl;
    out << "  //" << endl;
    out << "  //  operators" << endl;
    out << "  //" << endl;
    out << "  " << customType.name_ << "& operator=(const "
        << customType.name_ << "& p);" << endl;
    out << "  bool operator==(const " << customType.name_
        << "& p) const;" << endl;
    out << "  " << endl;

    out << "public:" << endl;
    out << "  rpc::DECODE_RESULT SerializeLoad(rpc::Decoder& decoder, "
           "io::MemoryStream& in);" << endl;
    out << "  void SerializeSave(rpc::Encoder& encoder, "
            "io::MemoryStream* out) const;" << endl;
    out << "  string ToString() const;;" << endl;
    out << "  rpc::Custom* Clone() const {" << endl;
    out << "    return new " << customType.name_ << "(*this);" << endl;
    out << "  }" << endl;
    out << "};" << endl;
    out << "" << endl;
    out << "" << endl;
  }
  out << "#endif //" << strFileIdentifier << endl;

  LOG_INFO << "Wrote file: " << types_h_file;
  return true;
}

string GetTypeFormName(const PType* ptype) {
  if ( ptype->name_ == RPC_VOID ) {
    return "";
  } else if ( ptype->name_ == RPC_BOOL ) {
    return "[ '__boolean' ]";
  } else if ( ptype->name_ == RPC_INT ||
              ptype->name_ == RPC_BIGINT ||
              ptype->name_ == RPC_FLOAT ) {
    return "[ '__number' ]";
  } else if ( ptype->name_ == RPC_STRING ) {
    return "[ '__text' ]";
  } else if ( ptype->name_ == RPC_MAP ) {
    return "[ '__map', " +
        GetTypeFormName(ptype->subtype1_) + ", " +
        GetTypeFormName(ptype->subtype2_) + " ]";
  } else if ( ptype->name_ == RPC_ARRAY ) {
    return "[ '__array', " + GetTypeFormName(ptype->subtype1_) + "]";
  }
  return "[ '" + ptype->name_ + "' ]";
}

void GetTypesToImport(const PType* ptype,
                      set<string>* typeset) {
  if ( ptype->name_ == RPC_VOID ||
       ptype->name_ == RPC_BOOL ||
       ptype->name_ == RPC_INT ||
       ptype->name_ == RPC_BIGINT ||
       ptype->name_ == RPC_FLOAT ||
       ptype->name_ == RPC_STRING ) {
    return;
  } else if ( ptype->name_ == RPC_MAP ) {
    if ( typeset->find(ptype->subtype1_->name_) == typeset->end() )
      GetTypesToImport(ptype->subtype1_, typeset);
    if ( typeset->find(ptype->subtype2_->name_) == typeset->end() )
      GetTypesToImport(ptype->subtype2_, typeset);
  } else if ( ptype->name_ == RPC_ARRAY ) {
    if ( typeset->find(ptype->subtype1_->name_) == typeset->end() )
      GetTypesToImport(ptype->subtype1_, typeset);
  } else {
    typeset->insert(ptype->name_);
  }
}

bool ExporterCC::ExportTypesCC(const char* include_h_file,
                               const char* types_cc_file) {
  ofstream out(types_cc_file, ios::out | ios::trunc);
  if ( !out.is_open() ) {
    LOG_ERROR << "Cannot open output file: " << types_cc_file
              << ", err: " << GetLastSystemErrorDescription();
    return false;
  }
  ExportAutoGenFileHeader(out);

  // include all needed files.
  out << "" << endl;
  out << "#include <whisperlib/common/base/errno.h>" << endl;
  out << "#include <whisperlib/common/base/log.h>" << endl;
  out << "#include <whisperlib/net/rpc/lib/codec/rpc_encoder.h>" << endl;
  out << "#include <whisperlib/net/rpc/lib/codec/rpc_decoder.h>" << endl;
  out << IncludeLine(include_h_file) << endl;
  out << "" << endl;

  for ( PCustomTypeArray::const_iterator it = customTypes_.begin();
        it != customTypes_.end(); ++it ) {
    const PCustomType& customType = *it;
    const PAttributeArray& attributes = customType.attributes_;

    out << customType.name_ << "::" << customType.name_ << "()" << endl;
    out << "    : rpc::Custom()";
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ++it) {
      const PAttribute& attribute = *it;
      out << "," << endl;
      out << "      " << attribute.name_ << "()";
    }
    out << endl;
    out << "{" << endl;
    out << "}" << endl;
    if ( !attributes.empty() ) {
      // generate constructor(args) only if args are not empty,
      // otherwise we already have constructor()
      out << customType.name_ << "::" << customType.name_ << "(";
      for ( PAttributeArray::const_iterator it = attributes.begin();
            it != attributes.end(); ) {
        const PAttribute& attribute = *it;
        out << TranslateType(attribute.type_) << " _" << attribute.name_;
        ++it;
        if ( it != attributes.end() ) {
          out << ", ";
        }
      }
      out << ")" << endl;
      out << "    : rpc::Custom()";
      for ( PAttributeArray::const_iterator it = attributes.begin();
            it != attributes.end(); ++it) {
        const PAttribute& attribute = *it;
        out << "," << endl;
        out << "      " << attribute.name_ << "(_" << attribute.name_ << ")";
      }
      out << endl;
      out << "{" << endl;
      out << "}" << endl;
    }
    out << customType.name_ << "::" << customType.name_ << "(const "
        << customType.name_ << "& _p_)" << endl;
    out << "    : rpc::Custom()";
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ++it) {
      const PAttribute& attribute = *it;
      out << "," << endl;
      out << "      " << attribute.name_ << "(_p_." << attribute.name_ << ")";
    }
    out << endl;
    out << "{" << endl;
    out << "}" << endl;
    out << customType.name_ << "::~" << customType.name_ << "()" << endl;
    out << "{" << endl;
    out << "}" << endl;
    out << "" << endl;

    out << "void " << customType.name_ << "::Fill(";
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ) {
      const PAttribute& attribute = *it;
      out << TranslateType(attribute.type_) << " _" << attribute.name_;
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
      out << "  " << attribute.name_ << " = _" << attribute.name_
          << ";" << endl;
    }
    out << "}" << endl;

    out << "void " << customType.name_ << "::FillRef(";
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ) {
      const PAttribute& attribute = *it;
      out << "const " << TranslateType(attribute.type_) << "& _"
          << attribute.name_;
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
      out << "  " << attribute.name_ << " = _" << attribute.name_
          << ";" << endl;
    }
    out << "}" << endl;
    out << "" << endl;

    out << "string  " << customType.name_
        << "::GetHTMLFormFunction() {" << endl;
    out << "  string s;" << endl;
    set<string> types_to_import;
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ++it ) {
      GetTypesToImport(&it->type_, &types_to_import);
    }
    for ( set<string>::const_iterator it = types_to_import.begin();
          it != types_to_import.end(); ++it ) {
      out << "  s += " + *it + "::GetHTMLFormFunction();" << endl;
      out << "  s += \"\\n\";" << endl;
    }
    out << "  s += \"function add_" << customType.name_
        << "_control(label, name, is_optional, f, elem) {\\n\";" << endl;
    out << "  s += \" if ( is_optional ) {\\n\";" << endl;
    out << "  s += \"   add_optional_element(f, elem);\\n\";" << endl;
    out << "  s += \" }\\n\";" << endl;
    out << "  s += \" var ul = document.createElement('ul');\\n\";" << endl;
    out << "  s += \" add_begin_struct(name, ul, null);\\n\";" << endl;
    out << "  s += \" if ( label != '' ) {\\n\";" << endl;
    out << "  s += \"   ul.appendChild(document.createTextNode"
        << "(label + ': '));\\n\";" << endl;
    out << "  s += \" }\\n\";" << endl;
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ++it ) {
      const PAttribute& attr = *it;
      if ( attr.type_.name_ == RPC_VOID ) {
      } else if ( attr.type_.name_ == RPC_BOOL ) {
        out << "  s += \"  add_boolean_control('" << attr.name_
            << "', '" << attr.name_ << "', "
            << (attr.isOptional_ ? "true" : "false")
            << ", ul, null);\\n\";" << endl;
      } else if ( attr.type_.name_ == RPC_INT ||
                  attr.type_.name_ == RPC_BIGINT ||
                  attr.type_.name_ == RPC_FLOAT ) {
        out << "  s += \"  add_number_control('" << attr.name_ << "', '"
            << attr.name_ << "', " << (attr.isOptional_ ? "true" : "false")
            << ", ul, null);\\n\";" << endl;
      } else if ( attr.type_.name_ == RPC_STRING ) {
        out << "  s += \"  add_text_control('" << attr.name_ << "', '"
            << attr.name_ << "', " << (attr.isOptional_ ? "true" : "false")
            << ", ul, null);\\n\";" << endl;
      } else if ( attr.type_.name_ == RPC_MAP ) {
        out << "  s += \"  add_map_control('" << attr.name_ << "', '"
            << attr.name_ <<  "', " << (attr.isOptional_ ? "true" : "false")
            << ", ul, null, " << GetTypeFormName(attr.type_.subtype1_)
            << ", " + GetTypeFormName(attr.type_.subtype2_)
            << ");\\n\";" << endl;
      } else if ( attr.type_.name_ == RPC_ARRAY ) {
        out << "  s += \"  add_array_control('" << attr.name_ << "', '"
            << attr.name_ << "', " << (attr.isOptional_ ? "true" : "false")
            << ", ul, null, " << GetTypeFormName(attr.type_.subtype1_)
            << ");\\n\";" << endl;
      } else {
        out << "  s += \"  add_" << attr.type_.name_ << "_control('"
            << attr.name_ <<  "', '" << attr.name_ << "', "
            << (attr.isOptional_ ? "true" : "false")
            << ", ul, null);\\n\";" << endl;
      }
    }
    out << "  s += \"  add_end_struct(name, ul, null);\\n\";" << endl;
    out << "  s += \"  ul.appendChild(document.createElement('hr'));\\n\";"
        << endl;
    out << "  s += \"  if ( elem == null ) {\\n\";" << endl;
    out << "  s += \"    f.appendChild(ul);\\n\";" << endl;
    out << "  s += \"  } else {\\n\";" << endl;
    out << "  s += \"    f.insertBefore(ul, elem)\\n\";" << endl;
    out << "  s += \"  }\\n\";" << endl;
    out << "  s += \"}\\n\";" << endl;
    out << "  return s;" << endl;
    out << "}" << endl;

    out << "////////////////////////////////////////////////////" << endl;
    out << "//" << endl;
    out << "//  operators" << endl;
    out << "//" << endl;
    out << customType.name_ << "& " << customType.name_
        << "::operator=(const " << customType.name_ << "& p)" << endl;
    out << "{" << endl;
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ++it ) {
      const PAttribute& attribute = *it;
      out << "  " << attribute.name_ << " = p." << attribute.name_
          << ";" << endl;
    }
    out << "  return *this;" << endl;
    out << "}" << endl;
    out << "bool " << customType.name_ << "::operator==(const "
        << customType.name_ << "& p) const" << endl;
    out << "{" << endl;
    out << "  return ";
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ) {
      const PAttribute& attribute = *it;
      out << "         " << attribute.name_ << " == p."
          << attribute.name_ << " ";

      ++it;
      if ( it != attributes.end() ) {
        out << "&&" << endl;
      }
    }
    if ( attributes.empty() ) {
      out << "true";  // return value if there are no attributes to tests
    }
    out << ";" << endl;
    out << "}" << endl;
    out << ""  << endl;
    out << "////////////////////////////////////////////////////" << endl;
    out << "//" << endl;
    out << "//  Serialization" << endl;
    out << "//" << endl;
    out << "rpc::DECODE_RESULT " << customType.name_ << "::SerializeLoad("
           "rpc::Decoder& decoder, io::MemoryStream& in) { " << endl;
    out << "  // unset all fields" << endl;
    out << "  //" << endl;
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ++it ) {
      const PAttribute& attribute = *it;
      out << "  " << attribute.name_ << ".reset();" << endl;
    }
    out << "  " << endl;
    out << "  rpc::DECODE_RESULT result = decoder.DecodeStructStart(in);" << endl;
    out << "  if ( result != rpc::DECODE_RESULT_SUCCESS ) {" << endl;
    out << "    return result; // bad or not enough data" << endl;
    out << "  }" << endl;
    out << "  " << endl;
    out << "  uint32 i = 0;" << endl;
    out << "  for ( i = 0; true; i++ ) {" << endl;
    out << "    bool hasMoreFields = false;" << endl;
    out << "    DECODE_VERIFY(decoder.DecodeStructContinue(in, &hasMoreFields));" << endl;
    out << "    if ( !hasMoreFields ) {" << endl;
    out << "      break;" << endl;
    out << "    }" << endl;
    out << "    " << endl;

    out << "    DECODE_VERIFY(decoder.DecodeStructAttribStart(in));" << endl;
    out << "    " << endl;

    out << "    // read field identifier" << endl;
    out << "    //" << endl;
    out << "    string fieldname;" << endl;
    out << "    result = decoder.Decode(in, &fieldname);" << endl;
    out << "    if ( result != rpc::DECODE_RESULT_SUCCESS ) {" << endl;
    out << "      return result; // bad or not enough data" << endl;
    out << "    }" << endl;
    out << "    " << endl;

    out << "    DECODE_VERIFY(decoder.DecodeStructAttribMiddle(in));" << endl;
    out << "    " << endl;

    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ++it ) {
      const PAttribute& attribute = *it;
      out << "    " << (it == attributes.begin() ? "" : "else ")
          << "if ( fieldname == \"" << attribute.name_ << "\" ) { " << endl;
      out << "      if ( " << attribute.name_ << ".is_set()) {" << endl;
      out << "        LOG_ERROR << \"Duplicate field '"
          << attribute.name_ << "'.\";" << endl;
      out << "        return rpc::DECODE_RESULT_ERROR;" << endl;
      out << "      }" << endl;
      out << "      result = decoder.Decode(in, " << attribute.name_
          << ".ptr());" << endl;
      out << "      if ( result != rpc::DECODE_RESULT_SUCCESS ) {" << endl;
      out << "        if ( result == rpc::DECODE_RESULT_NOT_ENOUGH_DATA ) {"
          << endl;
      out << "          LOG_DEBUG << \"Failed to decode field '"
          << attribute.name_ << "', result = \" << result;" << endl;
      out << "        } else {" << endl;
      out << "          LOG_ERROR << \"Failed to decode field '"
          << attribute.name_ << "', result = \" << result;" << endl;
      out << "        }" << endl;
      out << "        return result;" << endl;
      out << "      }" << endl;
      out << "    }" << endl;
    }
    if ( !attributes.empty() ) {
      out << "    else" << endl;
    }
    out << "    {" << endl;
    out << "      LOG_ERROR << \"Unknown field name: '\" << fieldname "
        << "<< \"', at position \" << i << \".\";" << endl;
    out << "      return rpc::DECODE_RESULT_ERROR;" << endl;
    out << "    };" << endl;
    out << "    DECODE_VERIFY(decoder.DecodeStructAttribEnd(in));" << endl;
    out << "  }" << endl;
    out << "  " << endl;

    out << "  // verify required fields" << endl;
    out << "  //" << endl;
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ++it ) {
      const PAttribute& attribute = *it;
      if ( attribute.isOptional_ ) {
        continue;
      }
      out << "  if ( !" << attribute.name_ << ".is_set() ) {" << endl;
      out << "    LOG_ERROR << \"Deserialization cannot find required field '"
          << attribute.name_ << "'\";" << endl;
      out << "    return rpc::DECODE_RESULT_ERROR;" << endl;
      out << "  }" << endl;
    }
    out << "  " << endl;
    out << "  // success" << endl;
    out << "  return rpc::DECODE_RESULT_SUCCESS;" << endl;
    out << "}" << endl;
    out << "" << endl;

    out << "void " << customType.name_ << "::SerializeSave("
           "rpc::Encoder& encoder, io::MemoryStream* out) const {" << endl;
    out << "  // verify required fields" << endl;
    out << "  //" << endl;
    unsigned nRequiredFields = 0;
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ++it ) {
      const PAttribute& attribute = *it;
      if ( attribute.isOptional_ ) {
        continue;
      }
      nRequiredFields++;
      out << "  CHECK(" << attribute.name_
          << ".is_set()) << \"Unset required field '"
          << attribute.name_ << "'\";" << endl;
    }
    out << "  " << endl;

    out << "  // proceed to serialization" << endl;
    out << "  // " << nRequiredFields
        << " required fields were already verified" << endl;
    out << "  uint32 fields = " << nRequiredFields;
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ++it ) {
      const PAttribute& attribute = *it;
      if ( !attribute.isOptional_ ) {  // ignore required fields
        continue;
      }
      out << endl << "                   + ("
          << attribute.name_ << ".is_set() ? 1 : 0)";
    }
    out << ";" << endl;
    out << "  encoder.EncodeStructStart(fields, out);" << endl;
    out << "  " << endl;

    out << "  uint32 count_encoded_attributes = 0;" << endl;
    out << "  (void)count_encoded_attributes;//prevent UNUSED warning" << endl;
    for ( PAttributeArray::const_iterator it = attributes.begin();
        it != attributes.end(); ++it ) {
      const PAttribute& attribute = *it;
      if ( attribute.isOptional_ ) {
        out << "  if (" << attribute.name_ << ".is_set())" << endl;
      } else {
        out << "  // if (" << attribute.name_
            << ".is_set())//REQUIRED field existence already verified" << endl;
      }
      out << "  {" << endl;
      out << "    if(count_encoded_attributes > 0) {" << endl;
      out << "      encoder.EncodeStructContinue(out);" << endl;
      out << "    }" << endl;
      out << "    encoder.EncodeStructAttribStart(out);" << endl;
      out << "    encoder.Encode(\"" << attribute.name_ << "\", out);" << endl;
      out << "    encoder.EncodeStructAttribMiddle(out);" << endl;
      out << "    encoder.Encode(" << attribute.name_ << ".get(), out);" << endl;
      out << "    encoder.EncodeStructAttribEnd(out);" << endl;
      out << "    count_encoded_attributes++;" << endl;
      out << "  }" << endl;
    }
    out << "  encoder.EncodeStructEnd(out);" << endl;
    out << "}" << endl;
    out << "" << endl;

    out << "///////////////////////////////////////////////////" << endl;
    out << "//" << endl;
    out << "//  Loggable interface methods                      */" << endl;
    out << "//" << endl;
    out << "string " << customType.name_ << "::ToString() const {" << endl;
    out << "  ostringstream oss;" << endl;
    out << "  oss << \"" << customType.name_ << "{\";" << endl;
    for ( PAttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ) {
      const PAttribute& attribute = *it;
      out << "  oss << \"" << attribute.name_ << "=\" << "
          << attribute.name_ << ".ToString()";
      ++it;
      if ( it != attributes.end() ) {
        out << " << \", \";" << endl;
      } else {
        out << ";" << endl;
      }
    }
    out << "  oss << \"}\";" << endl;
    out << "  return oss.str();" << endl;
    out << "}" << endl;
    out << "" << endl;
  }
  out << "" << endl;

  LOG_INFO << "Wrote file: " << types_cc_file;
  return true;
}

bool ExporterCC::ExportClientWrappersH(const char* clientWrappers_h_file) {
  ofstream out(clientWrappers_h_file, ios::out | ios::trunc);
  if ( !out.is_open() ) {
    LOG_ERROR << "Cannot open output file: " << clientWrappers_h_file;
    return false;
  }
  ExportAutoGenFileHeader(out);

  const string strFileIdentifier(MakeFileIdentifier(clientWrappers_h_file));
  out << "#ifndef " << strFileIdentifier << endl;
  out << "#define " << strFileIdentifier << endl;
  out << "" << endl;

  // include all needed files.
  //
  out << "#include <whisperlib/net/rpc/lib/client/rpc_service_wrapper.h>" << endl;
  out << IncludeLine(fClientTypes_ + ".h") << endl;
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

    out << "class ServiceWrapper" << serviceName
        << " : protected rpc::ServiceWrapper {" << endl;
    out << "public:" << endl;
    out << "  ServiceWrapper" << serviceName
        << "(rpc::IClientConnection& rpcConnection, "
           "const string& service_instance_name);" << endl;
    out << "  virtual ~ServiceWrapper" << serviceName << "();" << endl;
    out << "  " << endl;
    out << "  // public methods of rpc::ServiceWrapper. Deny access "
        << "to Call & Connect related methods." << endl;
    out << "  using rpc::ServiceWrapper::SetCallTimeout;" << endl;
    out << "  using rpc::ServiceWrapper::GetCallTimeout;" << endl;
    out << "  using rpc::ServiceWrapper::GetServiceClassName;" << endl;
    out << "  using rpc::ServiceWrapper::GetServiceName;" << endl;
    out << "  using rpc::ServiceWrapper::CancelCall;" << endl;
    out << "  " << endl;
    out << "  static const string& ServiceClassName();" << endl;
    out << "  " << endl;
    out << "  //////////////////////////////////////////////////" << endl;
    out << "  //" << endl;
    out << "  //  Service methods" << endl;
    out << "  //" << endl;
    const PFunctionArray& functions = service.functions_;
    for ( PFunctionArray::const_iterator it = functions.begin();
          it != functions.end(); ++it) {
      const PFunction& function = *it;
      const string& functionName = function.name_;
      const PType& functionReturn = function.output_;
      const PParamArray& args = function.input_;

      // The synchronized RPC method
      //
      out << "  void " << functionName << "(rpc::CallResult< "
          << TranslateType(functionReturn) << " > & ret";
      if ( !args.empty() ) {
        out << ", ";
      }
      for ( PParamArray::const_iterator it = args.begin();
            it != args.end(); ) {
        const PParam& arg = *it;
        out << "const " << TranslateType(arg.type_) << " &";

        ++it;
        if ( it != args.end() ) {
          out << ", ";
        }
      }
      out << ");" << endl;

      // The asynchronous RPC method with simple callback
      //
      out << "  rpc::CALL_ID " << functionName
          << "(Callback1<const rpc::CallResult< "
          << TranslateType(functionReturn) << " > &>* fun";
      if ( !args.empty() ) {
        out << ", ";
      }
      for ( PParamArray::const_iterator it = args.begin();
            it != args.end(); ) {
        const PParam& arg = *it;
        out << "const " << TranslateType(arg.type_) << " &";

        ++it;
        if ( it != args.end() ) {
          out << ", ";
        }
      }
      out << ");" << endl;
      out << "  " << endl;
    }
    out << "};" << endl;
    out << "" << endl;
  }

  out << "#endif //" << strFileIdentifier << endl;

  LOG_INFO << "Wrote file: " << clientWrappers_h_file;
  return true;
}

bool ExporterCC::ExportClientWrappersCC(const char* clientWrappers_cc_file) {
  ofstream out(clientWrappers_cc_file, ios::out | ios::trunc);
  if ( !out.is_open() ) {
    LOG_ERROR << "Cannot open output file: " << clientWrappers_cc_file;
    return false;
  }
  ExportAutoGenFileHeader(out);

  // include all needed files.
  //
  out << "" << endl;
  out << IncludeLine(fClientWrappers_ + ".h") << endl;
  out << "#include <whisperlib/common/base/scoped_ptr.h>" << endl;
  out << "" << endl;

  for ( PServiceArray::const_iterator it = services_.begin();
        it != services_.end(); ++it ) {
    const PService& service = *it;
    const string& serviceName = service.name_;
    const string& wrapperName = string("ServiceWrapper") + serviceName;

    out << "///////////////////////////////////////////////////" << endl;
    out << "//" << endl;
    out << "// " << wrapperName << endl;
    out << "//" << endl;
    out << wrapperName << "::" << wrapperName
        << "(rpc::IClientConnection& rpcConnection, "
           "const string& service_instance_name)" << endl;
    out << " : rpc::ServiceWrapper(rpcConnection, ServiceClassName(), "
           "service_instance_name) {" << endl;
    out << "}" << endl;
    out << wrapperName << "::~" << wrapperName << "() {" << endl;
    out << "}" << endl;
    out << ""  << endl;
    out << "const string& " << wrapperName << "::ServiceClassName() {" << endl;
    out << "  static const string k_str(\"" << serviceName << "\");" << endl;
    out << "  return k_str;" << endl;
    out << "}" << endl;
    out << ""  << endl;

    const PFunctionArray& functions = service.functions_;
    for ( PFunctionArray::const_iterator it = functions.begin();
          it != functions.end(); ++it ) {
      const PFunction& function = *it;
      const string& functionName = function.name_;
      const PType& functionReturn = function.output_;
      const PParamArray& args = function.input_;

      // The synchronized RPC method
      //
      out << "void " << wrapperName << "::" << functionName << "("
          "rpc::CallResult< " << TranslateType(functionReturn) << " > & ret";
      if ( !args.empty() ) {
        out << ", ";
      }
      for ( PParamArray::const_iterator it = args.begin(); it != args.end(); ) {
        const PParam& arg = *it;
        out << "const " << TranslateType(arg.type_) << " & " << arg.name_;

        ++it;
        if ( it != args.end() ) {
          out << ", ";
        }
      }
      out << ") {" << endl;
      out << "  io::MemoryStream __params;" << endl;
      out << "  scoped_ptr<rpc::Encoder> "
          << "encoder(CreateEncoder(GetCodec()));" << endl;
      out << "  encoder->EncodeArrayStart(" << args.size() << ", &__params);" << endl;
      unsigned nArgIndex = 0;
      for ( PParamArray::const_iterator it = args.begin();
            it != args.end(); nArgIndex++ ) {
        const PParam& arg = *it;
        out << "    encoder->Encode(" << arg.name_ << ", &__params);" << endl;

        ++it;
        if ( it != args.end() ) {
          out << "  encoder->EncodeArrayContinue(&__params);" << endl;
        }
      }
      out << "  encoder->EncodeArrayEnd(&__params);" << endl;
      out << "  const std::string __k_function_name(\""
           << functionName << "\");" << endl;
      out << "  Call(__k_function_name, __params, ret);" << endl;
      out << "}" << endl;

      // The asynchronous RPC method with simple callback
      //
      out << "rpc::CALL_ID " << wrapperName << "::" << functionName << "("
          << "Callback1<const rpc::CallResult< "
          << TranslateType(functionReturn) << " > &>* fun";
      if ( !args.empty() ) {
        out << ", ";
      }
      for ( PParamArray::const_iterator it = args.begin(); it != args.end(); ) {
        const PParam& arg = *it;
        out << "const " << TranslateType(arg.type_) << " & " << arg.name_;

        ++it;
        if ( it != args.end() ) {
          out << ", ";
        }
      }
      out << ") {" << endl;
      out << "  io::MemoryStream __params;" << endl;
      out << "  scoped_ptr<rpc::Encoder> "
          << "encoder(CreateEncoder(GetCodec()));" << endl;
      out << "  encoder->EncodeArrayStart(" << args.size() << ", &__params);" << endl;
      nArgIndex = 0;
      for ( PParamArray::const_iterator it = args.begin();
            it != args.end(); nArgIndex++ ) {
        const PParam& arg = *it;
        out << "    encoder->Encode(" << arg.name_ << ", &__params);" << endl;

        ++it;
        if ( it != args.end() ) {
          out << "  encoder->EncodeArrayContinue(&__params);" << endl;
        }
      }
      out << "  encoder->EncodeArrayEnd(&__params);" << endl;
      out << "  return AsyncCall(\"" << functionName << "\", __params, fun);"
          << endl;
      out << "}" << endl;
      out << "" << endl;
    }
    out << "" << endl;
  }

  LOG_INFO << "Wrote file: " << clientWrappers_cc_file;
  return true;
}

bool ExporterCC::ExportServerInvokersH(const char* serverInvokers_h_file) {
  ofstream out(serverInvokers_h_file, ios::out | ios::trunc);
  if ( !out.is_open() ) {
    LOG_ERROR << "Cannot open output file: "
              << serverInvokers_h_file;
    return false;
  }
  ExportAutoGenFileHeader(out);

  const string strFileIdentifier(MakeFileIdentifier(serverInvokers_h_file));
  out << "#ifndef " << strFileIdentifier << endl;
  out << "#define " << strFileIdentifier << endl;
  out << endl;

  // include all needed files.
  //
  out << "#include <whisperlib/net/rpc/lib/types/rpc_all_types.h>" << endl;
  out << "#include <whisperlib/net/rpc/lib/types/rpc_message.h>" << endl;
  out << "#include <whisperlib/net/rpc/lib/server/rpc_service_invoker.h>" << endl;
  out << IncludeLine(fServerTypes_    + ".h") << endl;
  out << endl;

  string verbatim = GetVerbatim();
  if ( !verbatim.empty() ) {
    out << "// Verbatim stuff included:" << endl
        << verbatim << endl;
  }

  for ( PServiceArray::const_iterator it = services_.begin();
        it != services_.end(); ++it ) {
    const PService& service = *it;
    const string& serviceName = service.name_;
    const string invokerName = string("ServiceInvoker") + serviceName;
    const PFunctionArray& functions = service.functions_;

    out << "class " << invokerName << " : public rpc::ServiceInvoker {" << endl;
    out << "public:" << endl;
    out << "  " << invokerName << "(const string& instance_name);" << endl;
    out << "  virtual ~" << invokerName << "();" << endl;
    out << "" << endl;

    out << "  static const string& ServiceClassName();" << endl;
    out << "" << endl;

    out << "  virtual string GetTurntablePage(const string& base_path, "
        << "const string& url_path) const;" << endl;
    for ( PFunctionArray::const_iterator it = functions.begin();
          it != functions.end(); ++it ) {
      const PFunction& function = *it;
      const string& functionName = function.name_;
      out << "  string Get" << functionName
          << "Form(const string& command_base_path) const;" << endl;
    }
    out << "" << endl;

    out << "protected:" << endl;
    out << "  bool Call(rpc::Query* query);" << endl;
    out << "" << endl;

    out << "  //////////////////////////////////////////////////" << endl;
    out << "  //" << endl;
    out << "  //  " << serviceName
        << " service methods, to be implemented in subclass" << endl;
    out << "  //" << endl;
    for ( PFunctionArray::const_iterator it = functions.begin();
          it != functions.end(); ++it ) {
      const PFunction& function = *it;
      const string& functionName = function.name_;
      const string& functionReturnType = TranslateType(function.output_);
      const PParamArray& args = function.input_;
      out << " private: " << endl;
      out << "  class QueryParams_" << functionName
          << " : public rpc::Query::Params { " << endl;
      out << "   public: " << endl;
      for ( PParamArray::const_iterator it = args.begin();
            it != args.end(); ++it ) {
        const PParam& arg = *it;
        out << "    " << TranslateType(arg.type_)
            << " " << arg.name_ << "_;" << endl;
      }
      out << "  };" << endl;
      out << " public: " << endl;
      out << "  virtual void "<< functionName << "(rpc::CallContext< "
          << functionReturnType << " >* call";
      for ( PParamArray::const_iterator it = args.begin();
            it != args.end(); ++it ) {
        const PParam& arg = *it;
        if ( IsSimpleType(arg.type_) ) {
          // Pass by value
          out << ", " << TranslateType(arg.type_) << " " << arg.name_;
        } else {
          // Pass by reference (just for performance)
          out << ", const " << TranslateType(arg.type_) << "& " << arg.name_;
        }
      }
      out << ") = 0;" << endl;
    }
    out << "};" << endl;
    out << ""   << endl;
  }
  out << "" << endl;

  out << "#endif //" << strFileIdentifier << endl;

  LOG_INFO << "Wrote file: " << serverInvokers_h_file;
  return true;
}

bool ExporterCC::ExportServerInvokersCC(const char* serverInvokers_cc_file) {
  ofstream out(serverInvokers_cc_file, ios::out | ios::trunc);
  if ( !out.is_open() ) {
    LOG_ERROR << "Cannot open output file: " << serverInvokers_cc_file;
    return false;
  }
  ExportAutoGenFileHeader(out);

  // include all needed files.
  //
  out << "#include <whisperlib/common/base/log.h>" << endl;
  out << "#include <whisperlib/common/base/errno.h>" << endl;
  out << "#include <whisperlib/net/rpc/lib/types/rpc_all_types.h>" << endl;
  out << "#include <whisperlib/net/rpc/lib/server/rpc_services_manager.h>" << endl;

  out << IncludeLine(fServerInvokers_ + ".h") << endl;
  out << endl;

  for ( PServiceArray::const_iterator it = services_.begin();
        it != services_.end(); ++it) {
    const PService& service = *it;
    const string& serviceName = service.name_;
    const string invokerName = string("ServiceInvoker") + serviceName;
    const PFunctionArray& functions = service.functions_;

    out << invokerName << "::" << invokerName
        << "(const string& instance_name)" << endl
        << "  : rpc::ServiceInvoker(ServiceClassName(), instance_name){}"
        << endl;
    out << invokerName << "::~" << invokerName << "(){}" << endl;
    out << "" << endl;

    out << "const string& " << invokerName << "::ServiceClassName() {" << endl;
    out << "  static const string k_str(\"" << service.name_ << "\");" << endl;
    out << "  return k_str;" << endl;
    out << "}" << endl;
    out << "" << endl;

    out << "string " << invokerName << "::GetTurntablePage("
        << "const string& base_path, const string& url_path) const {" << endl;
    for ( PFunctionArray::const_iterator it = functions.begin();
          it != functions.end(); ++it ) {
      const PFunction& function = *it;
      const string& functionName = function.name_;
      out << "  if ( url_path == \"__form_" + functionName + "\" ) {"<< endl;
      out << "    return Get" + functionName + "Form(base_path);"<< endl;
      out << "  }" << endl;
    }
    out << "  if ( url_path != \"__forms\" ) {" << endl;
    out << "    return \"\";"<< endl;
    out << "  }" << endl;
    out << "  string s = \"<ul>\\n\";" << endl;
    for ( PFunctionArray::const_iterator it = functions.begin();
          it != functions.end(); ++it ) {
      const PFunction& function = *it;
      const string& functionName = function.name_;
      out << "  s += \"<a href=\\\"\" + base_path + \"/" << serviceName
          << "/__form_" << functionName << "\\\">" << functionName
          << "</a><br>\\n\";" << endl;
    }
    out << "  s += \"</ul>\\n\";" << endl;
    out << "  return s;" << endl;
    out << "}" << endl;

    for ( PFunctionArray::const_iterator it = functions.begin();
          it != functions.end(); ++it ) {
      const PFunction& function = *it;
      const string& functionName = function.name_;
      const string functionReturn = TranslateType(function.output_);
      const PParamArray& args = function.input_;

      out << "string " << invokerName << "::Get" << functionName
          << "Form(const string& command_base_path) const {" << endl;
      out << "  string s;" << endl;
      out << "  string aux;" << endl;
      out << "  s += \"<h1>" << service.name_ << " :: " <<  functionName
          << "</h1>\\n\";" << endl;
      out << "  s += \"<a href=\\\"\" + command_base_path + \"/"
          << serviceName << "/__forms\\\">all commands...</a><br>\\n\";"
          << endl;
      out << "  s += \"<p><form>\\n\";" << endl;
      out << "  s += \"<input type='hidden' name='bottom_page'>\\n\";" << endl;
      out << "  s += \"<script language=\\\"JavaScript1.1\\\">\\n\";" << endl;
      out << "  s += \"  f = document.getElementsByTagName('form')[0];\\n\";"
          << endl;
      out << "  s += \"  elem = document.getElementsByTagName"
          << "('hidden')[0];\\n\";" << endl;
      out << "  s += \"  add_begin_array('', f, elem);\\n\";" << endl;
      set<string> typeset;
      for ( PParamArray::const_iterator it = args.begin();
            it != args.end(); ++it ) {
        const PParam& arg = *it;
        GetTypesToImport(&arg.type_, &typeset);
        if ( arg.type_.name_ == RPC_VOID ) {
          // Nothing here
        } else if ( arg.type_.name_ == RPC_BOOL ) {
          out << "  s += \"  add_boolean_control('" << arg.name_
              << "', '', false, f, elem);\\n\";" << endl;
        } else if ( arg.type_.name_ == RPC_INT ||
                    arg.type_.name_ == RPC_BIGINT ||
                    arg.type_.name_ == RPC_FLOAT ) {
          out << "  s += \"  add_number_control('" << arg.name_
              << "', '', false, f, elem);\\n\";" << endl;
        } else if ( arg.type_.name_ == RPC_STRING ) {
          out << "  s += \"  add_text_control('" << arg.name_
              << "', '', false, f, elem);\\n\";" << endl;
        } else if ( arg.type_.name_ == RPC_MAP ) {
          out << "  s += \"  add_map_control('" << arg.name_
              << "', '', false, f, elem, "
              << GetTypeFormName(arg.type_.subtype1_)
              << ", "
              << GetTypeFormName(arg.type_.subtype2_) << ");\\n\";" << endl;
        } else if ( arg.type_.name_ == RPC_ARRAY ) {
          out << "  s += \"  add_array_control('" << arg.name_
              << "', '', false, f, elem, "
              << GetTypeFormName(arg.type_.subtype1_)
              << ");\\n\";" << endl;
        } else {
          out << "  s += \"  add_" << arg.type_.name_
              << "_control('" << arg.name_
              << "', '', false, f, elem);\\n\";" << endl;
        }
      }
      out << "  s += \"  add_end_array('', f, elem);\\n\";" << endl;
      out << "  s += \"</script>\";" << endl;
      out << "  s += \"<input type='button' name='__send__' "
          << " value='Send Command' "
          << "  onclick='var x = extract_form_data(parentNode, 0, "
          << " parentNode.elements.length, false)[0]; "
          << "  rpc_raw_query(\\\"\" + command_base_path + \"\\\", \\\""
          << service.name_ << "\\\", \\\"" << functionName << "\\\", x, "
          << "  display_rpc_return)'>\\n\";" << endl;
      out << "  s += \"<br>\";" << endl;
      out << "  s += \"</form>\";" << endl;
      // out << "  s += \"<div id=\\\"LOGAREA\\\"></div>\\\n\";" << endl;

      out << "  aux += \"<script language=\\\"JavaScript1.1\\\">\\n\";" << endl;
      for ( set<string>::const_iterator it = typeset.begin();
            it != typeset.end(); ++it ) {
        out << " aux += \"\\n\" + " << *it
            << "::GetHTMLFormFunction();" << endl;
      }
      out << "  aux += \"</script>\";" << endl;
      out << "  return aux + s;" << endl;
      out << "}" << endl;
    }
    out << "" << endl;

    out << "bool " << invokerName << "::Call(rpc::Query* query) {" << endl;
    out << "  CHECK_EQ(query->service(), GetName()); "
        << " // service should have already been verified" << endl;
    out << "  LOG_DEBUG << \"Call: method=\" << query->method();" << endl;
    out << "  " << endl;

    out << "  if ( !query->InitDecodeParams() ) {" << endl;
    out << "    LOG_ERROR << \"Failed to initialize params decoding\";" << endl;
    out << "    query->Complete(rpc::RPC_GARBAGE_ARGS);" << endl;
    out << "    return true;" << endl;
    out << "  }";
    out << "  " << endl;

    for ( PFunctionArray::const_iterator it = functions.begin();
          it != functions.end(); ++it ) {
      const PFunction& function = *it;
      const string& functionName = function.name_;
      const string functionReturn = TranslateType(function.output_);
      const PParamArray& args = function.input_;

      out << "  if ( query->method() == \"" << functionName << "\" ) {" << endl;
      out << "    // decode expected parameters" << endl;
      out << "    //" << endl;
      out << "    QueryParams_" << functionName << "* query_params "
          << "= new QueryParams_" << functionName << "();" << endl;
      out << "    query->SetParams(query_params);" << endl;
      for ( PParamArray::const_iterator it = args.begin();
            it != args.end(); ++it ) {
        const PParam& arg = *it;
        const string type_name = TranslateType(arg.type_);
        const string arg_name = arg.name_ + "_";
        out << "    if ( !query->DecodeParam(query_params->" << arg_name
            << ") ) { " << endl;
        out << "      DLOG_WARNING << \"Invoke '" << functionName
            << "' with garbage arguments. Unable to decode '"
            << arg.name_ << "': \" << query->RewindParams().DebugString();" << endl;
        out << "      query->Complete(rpc::RPC_GARBAGE_ARGS);" << endl;
        out << "      return true;" << endl;
        out << "    }" << endl;
      }
      out << "    if ( query->HasMoreParams() ) { " << endl;
      out << "      DLOG_WARNING << \"Invoke '"
          << functionName << "' with too many args: \" "
          << "<< query->RewindParams().DebugString();" << endl;
      out << "      query->Complete(rpc::RPC_GARBAGE_ARGS);" << endl;
      out << "      return true;" << endl;
      out << "    }" << endl;
      out << "    " << endl;
      out << "    DLOG_INFO << \"Invoke " << invokerName
          << "::" << functionName << "(\"";
      for ( PParamArray::const_iterator it = args.begin();
            it != args.end(); ) {
        const PParam& arg = *it;
        out << " << rpc::ToString(query_params->" << arg.name_ << "_) ";
        ++it;
        if ( it != args.end() ) {
          out << " << \", \"";
        }
      }
      out << " << \")\";" << endl;
      out << "    " << functionName << "(new rpc::CallContext< "
          << functionReturn << " >(query)";
      for ( PParamArray::const_iterator it = args.begin();
            it != args.end(); ++it ) {
        const PParam& arg = *it;
        out << ", query_params->" << arg.name_ << "_";
      }
      out << ");" << endl;
      out << "    return true;" << endl;
      out << "  }" << endl;
      out << endl;
    }

    out << "  // default" << endl;
    out << "  LOG_WARNING << \"No such method '\" << query->method() "
        << "<< \"' in service " << serviceName << "\";" << endl;
    out << "  query->Complete(rpc::RPC_PROC_UNAVAIL);" << endl;
    out << "  return true;" << endl;
    out << "}" << endl;
    out << ""   << endl;
  }

  LOG_INFO << "Wrote file: " << serverInvokers_cc_file;
  return true;
}

ExporterCC::ExporterCC(const PCustomTypeArray& customTypes,
                       const PServiceArray& services,
                       const PVerbatimArray& verbatim,
                       const vector<string>& inputFilenames)
    : Exporter(kLanguageName, customTypes, services, verbatim, inputFilenames),
      fClientTypes_(),
      fClientWrappers_(),
      fServerTypes_(),
      fServerInvokers_() {
}
ExporterCC::~ExporterCC() {
}

////////////////////////////////////////////////////////////////////////
//
//                    Exporter interface methods
//

const Keywords& ExporterCC::GetRestrictedKeywords() const {
  return ExporterCC::kRestrictedKeywords;
}

bool ExporterCC::SetParams(const map<string, string>& paramValues) {
#define APPEND_PARAM(pname, pval, mkabspath) do {               \
    map<string, string>::const_iterator it                      \
        = paramValues.find(pname);                              \
    if ( it != paramValues.end() ) {                            \
      if ( mkabspath ) {                                        \
        pval = io::MakeAbsolutePath(it->second.c_str());        \
      } else {                                                  \
        pval = it->second;                                      \
      }                                                         \
    }                                                           \
  } while ( false );

  APPEND_PARAM(paramClientTypes_, fClientTypes_, true);
  APPEND_PARAM(paramClientWrappers_, fClientWrappers_, true);
  APPEND_PARAM(paramServerTypes_, fServerTypes_, true);
  APPEND_PARAM(paramServerInvokers_, fServerInvokers_, true);

  APPEND_PARAM(paramIncludePathCut_, includePathCut_, false);
  APPEND_PARAM(paramIncludePathPaste_, includePathPaste_, false);
#undef APPEND_PARAM

  // check for missing parameters.
  // All client parameters should go together: all or none.
  // Same for server parameters. This is important because
  // serverInvokers.cc must include serverTypes.h.

  bool missingParams = false;
  if ( fClientTypes_.empty() ^ fClientWrappers_.empty() ) {
    LOG_ERROR << "If you specify one client parameter, you must specify both";
    missingParams = true;
  }
  if ( fServerTypes_.empty() ^ fServerInvokers_.empty() ) {
    LOG_ERROR << "If you specify one server parameter, you must specify both";
    missingParams = true;
  }

  return !missingParams;
}

bool ExporterCC::Export() {
  if ( !fClientTypes_.empty() ) {
    if ( !ExportTypesH((fClientTypes_ + ".h").c_str()) ) {
      return false;
    }
    if ( !ExportTypesCC((fClientTypes_ + ".h").c_str(),
                        (fClientTypes_ + ".cc").c_str()) ) {
      return false;
    }
  }
  if ( !fClientWrappers_.empty() ) {
    if ( !ExportClientWrappersH ((fClientWrappers_ + ".h").c_str()) )
      return false;
    if ( !ExportClientWrappersCC((fClientWrappers_ + ".cc").c_str()) )
      return false;
  }
  if ( !fServerTypes_.empty() ) {
    if ( !ExportTypesH ((fServerTypes_ + ".h").c_str()) )
      return false;
    if ( !ExportTypesCC((fServerTypes_ + ".h").c_str(),
                        (fServerTypes_ + ".cc").c_str()) )
      return false;
  }
  if ( !fServerInvokers_.empty() ) {
    if ( !ExportServerInvokersH ((fServerInvokers_ + ".h").c_str()) )
      return false;
    if ( !ExportServerInvokersCC((fServerInvokers_ + ".cc").c_str()) )
      return false;
  }
  return true;
}
}
