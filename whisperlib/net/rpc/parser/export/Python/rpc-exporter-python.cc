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
// Author: Catalin Popescu
//

#include <fstream>

#include "common/base/log.h"
#include "net/rpc/parser/version.h"
#include "net/rpc/parser/export/rpc-base-types.h"
#include "net/rpc/parser/export/Python/rpc-exporter-python.h"
#include "common/base/strutil.h"
#include "common/io/ioutil.h"


namespace rpc {

ExporterPython::ExporterPython(const PCustomTypeArray& customTypes,
                               const PServiceArray& services,
                               const PVerbatimArray& verbatim,
                               const vector<string>& inputFilenames)
    : Exporter(kLanguageName, customTypes, services, verbatim, inputFilenames) {
}
ExporterPython::~ExporterPython() {
}
//static
const string ExporterPython::kLanguageName("Python");

// static
const map<string, string> ExporterPython::kBaseTypesCorrespondence(
    ExporterPython::BuildBaseTypesCorrespondenceMap());

// static
map<string, string> ExporterPython::BuildBaseTypesCorrespondenceMap() {
  map<string, string> btc;
  btc[RPC_VOID]   = "None";
  btc[RPC_BOOL]   = "bool";
  btc[RPC_INT]    = "int";
  btc[RPC_BIGINT] = "long";
  btc[RPC_FLOAT]  = "float";
  btc[RPC_STRING] = "str";
  btc[RPC_ARRAY]  = "list";
  btc[RPC_MAP]    = "dict";
  return btc;
}

// static
const map<string, string>& ExporterPython::BaseTypeCorrespondence() {
  return kBaseTypesCorrespondence;
}

const Keywords& ExporterPython::GetRestrictedKeywords() const {
  return ExporterPython::kRestrictedKeywords;
}

//static
const Keywords ExporterPython::kRestrictedKeywords(
    ExporterPython::BuildRestrictedKeywords());

//static
Keywords ExporterPython::BuildRestrictedKeywords() {
  const char* res[] = {
    "and",       "del",       "from",      "not",       "while",
    "as",        "elif",      "global",    "or",        "with",
    "assert",    "else",      "if",        "pass",      "yield",
    "break",     "except",    "import",    "print",
    "class",     "exec",      "in",        "raise",    "continue",
    "finally",   "is",        "return",    "def",       "for",
    "lambda",    "try",
    "dict", "int", "float", "long", "list", "unicode", "str", "datetime",
    "None", "whisperrpc",
  };
  set<string> restricted;
  for ( unsigned i = 0; i < sizeof(res)/sizeof(res[0]); i++ ) {
    restricted.insert(res[i]);
  }
  return Keywords(kLanguageName, restricted, true);
}

//static
const Keywords& ExporterPython::RestrictedKeywords() {
  return kRestrictedKeywords;
}

const char ExporterPython::paramExport_[] = "file";

const map<string, string> ExporterPython::paramsDescription_(
    ExporterPython::BuildParamsDescription());

// static
const map<string, string> ExporterPython::BuildParamsDescription() {
  map<string, string> pd;
  pd[string(paramExport_)]    = string("{.py} Custom types and wrapper file.");
  return pd;
}

// static
const map<string, string>& ExporterPython::ParamsDescription() {
  return paramsDescription_;
}

bool ExporterPython::SetParams(const map<string, string>& paramValues) {
  const map<string, string>::const_iterator it_types =
      paramValues.find(paramExport_);
  if ( it_types != paramValues.end() ) {
    fExport_ = it_types->second;
  }
  return true;
}

void ExporterPython::ExportAutoGenFileHeader(ostream& out) {
  out << "#!/usr/bin/python" << endl;
  out << "# " << APP_NAME_AND_VERSION << endl;
  out << "# " << endl;
  out << "# This is an auto generated file. DO NOT MODIFY !" << endl;
  out << "# The source files are: " << endl;
  for ( vector<string>::const_iterator it = inputFilenames_.begin();
        it != inputFilenames_.end(); ++it ) {
    const string& filename = *it;
    out << "#  - " <<  io::MakeAbsolutePath(filename.c_str()) << endl;
  }
  out << "#" << endl;
  out << "import json" << endl;
  out << "import datetime" << endl;
  out << "import whisperrpc" << endl;
}

string GetSetterCode(const PAttribute& attr) {
  const char* p = attr.name_.c_str();
  if ( attr.type_.name_ == "void" ) {
    return strutil::StringPrintf(
        "  def set_%s(self, p_%s):\n"
        "    if p_%s is None:\n"
        "      self.__data['%s'] = None\n"
        "    else:\n"
        "      raise RpcTypeException('For member \"%s\"')\n",
        p, p, p, p, p);
  }

  string s = strutil::StringPrintf(
      "  def set_%s(self, p_%s):\n"
      "    if p_%s is None:\n"
      "      if self.__data.has_key('%s'):\n"
      "        del self.__data['%s']\n",
      p, p, p, p, p);

  if ( attr.type_.name_ ==  "bool" ) {
    s += strutil::StringPrintf(
        "    else:\n"
        "      if not isinstance(p_%s, bool):\n"
        "        raise RpcTypeException('For member \"%s')\n"
        "      self.__data['%s'] = p_%s\n",
        p, p, p, p);
  } else if ( attr.type_.name_ ==  "int" ) {
    s += strutil::StringPrintf(
        "    else:\n"
        "      if not isinstance(p_%s, int):\n"
        "        raise RpcTypeException('For member \"%s')\n"
        "      self.__data['%s'] = p_%s\n",
        p, p, p, p);
  } else if ( attr.type_.name_ ==  "bigint" ) {
    s += strutil::StringPrintf(
        "    else:\n"
        "      if (not isinstance(p_%s, int) and\n"
        "          not isinstance(p_%s, long)):\n"
        "        raise RpcTypeException('For member \"%s')\n"
        "      self.__data['%s'] = long(p_%s)\n",
        p, p, p, p, p);
  } else if ( attr.type_.name_ ==  "float" ) {
    s += strutil::StringPrintf(
        "    else:\n"
        "      if (not isinstance(p_%s, int) and\n"
        "          not isinstance(p_%s, float) and\n"
        "          not isinstance(p_%s, long)):\n"
        "        raise RpcTypeException('For member \"%s')\n"
        "      self.__data['%s'] = float(p_%s)\n",
        p, p, p, p, p, p);
  } else if ( attr.type_.name_ ==  "string" ) {
    s += strutil::StringPrintf(
        "    else:\n"
        "      if (not isinstance(p_%s, str) and\n"
        "          not isinstance(p_%s, unicode)):\n"
        "        raise RpcTypeException('For member \"%s')\n"
        "      self.__data['%s'] = p_%s\n",
        p, p, p, p, p);
  } else if ( attr.type_.name_ ==  "array" ) {
    // Minimal check ...
    s += strutil::StringPrintf(
        "    else:\n"
        "      if not isinstance(p_%s, list):\n"
        "        raise RpcTypeException('For member \"%s')\n"
        "      self.__data['%s'] = p_%s\n",
        p, p, p, p);
  } else if ( attr.type_.name_ ==  "map" ) {
    // Minimal check ...
    s += strutil::StringPrintf(
        "    else:\n"
        "      if not isinstance(p_%s, map):\n"
        "        raise RpcTypeException('For member \"%s')\n"
        "      self.__data['%s'] = p_%s\n",
        p, p, p, p);
  } else if ( attr.type_.name_ ==  "date" ) {
    // Minimal check ...
    s += strutil::StringPrintf(
        "    else:\n"
        "      if isinstance(p_%s, datetime.datetime):\n"
        "        self.__data['%s'] = p_%s\n"
        "      elif isinstance(p_%s, float):\n"
        "        self.__data['%s'] = datetime.datetime.fromtimestamp(p_%s)\n"
        "      else:\n"
        "        raise RpcTypeException('For member \"%s')\n",
        p, p, p, p, p, p, p);
  } else {
    s += strutil::StringPrintf(
        "    else:\n"
        "      if not isinstance(p_%s, %s):\n"
        "        if isinstance(p_%s, dict):\n"
        "          __p = %s()\n"
        "          __p.SetFromDict(p_%s)\n"
        "          self.__data['%s'] = __p\n"
        "        else:\n"
        "          raise RpcTypeException('For member \"%s\"')\n"
        "      else:\n"
        "        self.__data['%s'] = p_%s\n",
        p, attr.type_.name_.c_str(),
        p, attr.type_.name_.c_str(),
        p, p, p, p, p);
  }
  return s;
}

bool ExporterPython::Export() {
  if ( fExport_.empty() ) {
    return true;
  }

  ofstream out(fExport_.c_str(), ios::out | ios::trunc);
  if ( !out.is_open() ) {
    LOG_ERROR << "Cannot open output file: " << fExport_;
    return false;
  }
  ExportAutoGenFileHeader(out);
  out << endl;

  string verbatim = GetVerbatim();
  if ( !verbatim.empty() ) {
    out << "# Verbatim stuff included:" << endl
        << verbatim << endl;
  }
  out << "####################################################" << endl
      << "#" << endl
      << "# TYPES :" << endl
      << "#" << endl;

  for ( PCustomTypeArray::const_iterator it = customTypes_.begin();
        it != customTypes_.end(); ++it ) {
    const PCustomType& t = *it;
    const PAttributeArray& attr = t.attributes_;

    vector<string> args, initors, getters, testers, setters, verifiers;
    for (PAttributeArray::const_iterator it_attr = attr.begin();
         it_attr != attr.end(); ++it_attr) {
      const char* p = it_attr->name_.c_str();
      const string local(string("_") + it_attr->name_);
      const char* l =local.c_str();
      args.push_back(strutil::StringPrintf("               %s = None", l));
      initors.push_back(strutil::StringPrintf(
                            "    self.set_%s(%s)", p, l));
      getters.push_back(strutil::StringPrintf(
                            "  def get_%s(self):\n"
                            "    return self.__data.get('%s', None)\n",
                            p, p));
      setters.push_back(GetSetterCode(*it_attr));
      testers.push_back(strutil::StringPrintf(
                            "  def is_%s_set(self):\n"
                            "    return self.__data.has_key('%s')\n",
                            p, p));
      if ( !it_attr->isOptional_ ) {
        verifiers.push_back(strutil::StringPrintf(
                                "    if not self.is_%s_set():\n"
                                "      return False", p));
      }
    }
    // Class name and constructor
    out << "#######################################################" << endl
        << endl
        << endl;
    out << "class " << t.name_ << ":" << endl;
    if ( args.empty() ) {
      out << "  def __init__(self):" << endl;
    } else {
      out << "  def __init__(self," << endl << strutil::JoinStrings(args, ",\n")
          << "):" << endl;
    }
    out << "    self.__data = {}" << endl;
    out << strutil::JoinStrings(initors, "\n") << endl;

    out << "  def __str__(self): " << endl
        << "    return str(self.__data)" << endl;

    out << strutil::JoinStrings(getters, "\n") << endl << endl;
    out << strutil::JoinStrings(testers, "\n") << endl << endl;
    out << strutil::JoinStrings(setters, "\n") << endl << endl;

    //
    // IsValid
    //
    out << "  def IsValid(self):" << endl
        << strutil::JoinStrings(verifiers, "\n") << endl
        << "    return True" << endl
        << endl;

    //
    // JsonDecode
    //
    out << "  def JsonDecode(self, data):" << endl
        << "    self.__data = {}" << endl
        << "    x = json.loads(data)" << endl
        << "    if not isinstance(x, dict):" << endl
        << "      raise RpcTypeException('For decoded data')" << endl
        << "    self.SetFromDict(x)" << endl
        << endl;

    //
    // JsonEncode
    //
    out << "  def JsonEncode(self):" << endl
        << "    s = []" << endl;
    for (PAttributeArray::const_iterator it_attr = attr.begin();
         it_attr != attr.end(); ++it_attr) {
      const char* p = it_attr->name_.c_str();
      if ( !it_attr->isOptional_ ) {
        out << "    # " << p << " is required field:" << endl
            << "    if not self.is_" << p << "_set():" << endl
            << "      raise RpcMemberException('\"" << p
            << "\" is not set')" << endl
            << "    s.append('\"" << p << "\": %s' % "
            <<               "whisperrpc.JsonEncode(self.get_" << p << "()))" << endl;
      } else {
        out << "    # " << p << " is *not*required field:" << endl
            << "    if self.is_" << p << "_set():" << endl
            << "      s.append('\"" << p << "\": %s' % "
            <<               "whisperrpc.JsonEncode(self.get_" << p << "()))" << endl;
      }
    }
    out << "    return '{%s}' % ', '.join(s)" << endl
        << endl;

    //
    // SetFromDict
    //
    out << "  def SetFromDict(self, x):" << endl;
    for (PAttributeArray::const_iterator it_attr = attr.begin();
         it_attr != attr.end(); ++it_attr) {
      const char* p = it_attr->name_.c_str();
      out << "    if x.has_key('" << p << "'):" << endl
          << "      self.set_" << p << "(x.get('" << p << "', None))" << endl
          << "    else:" << endl
          << "      self.set_" << p << "(x.get(u'" << p << "', None))" << endl;
    }
    if ( attr.empty() ) {
      out << "    pass" << endl
          << endl;
    }
    out << endl
        << endl;
  }

  out << "####################################################" << endl
      << "#" << endl
      << "# SERVICES :" << endl
      << "#" << endl;
  for ( PServiceArray::const_iterator it = services_.begin();
        it != services_.end(); ++it ) {
    const PService& service = *it;
    const string& serviceName = service.name_;
    out << "class RpcService_" << serviceName << ":" << endl
        << "  def __init__(self, conn, url_base):" << endl
        << "    \"conn should be whisperrpc.RpcConnection / http.client\"" << endl
        << "    self.__conn = conn" << endl
        << "    self.__service_name = '" << serviceName << "'" << endl
        << "    self.__url_base = url_base" << endl
        << endl;

    const PFunctionArray& functions = service.functions_;
    for ( PFunctionArray::const_iterator it_fun = functions.begin();
          it_fun != functions.end(); ++it_fun ) {
      const PFunction& function = *it_fun;
      const string& functionName = function.name_;
      const PType& functionReturn = function.output_;
      const PParamArray& args = function.input_;

      // out << "  \"" << functionReturn << " " << functionName << "(";
      vector<string> comment_params, params, preparation;
      for ( PParamArray::const_iterator it_arg = args.begin();
            it_arg != args.end(); ++it_arg ) {
        comment_params.push_back(
            strutil::StringPrintf("%s %s",
                                  it_arg->type_.name_.c_str(),
                                  it_arg->name_.c_str()));
        params.push_back(
            strutil::StringPrintf("%s", it_arg->name_.c_str()));
        preparation.push_back(
            strutil::StringPrintf(
                "    __s.append(whisperrpc.JsonEncode(%s))",
                it_arg->name_.c_str()));
      }
      PAttribute retAttr;
      retAttr.type_ = functionReturn;
      retAttr.name_ = functionName + "_RetVal";

      out << GetSetterCode(retAttr) << endl
          << endl;
      if ( params.empty() ) {
        out << "  def " << functionName << "(self):" << endl;
      } else {
        out << "  def " << functionName << "(self, "
            <<                             strutil::JoinStrings(params, ", ")
            <<                             "):" << endl;
      }
      out << "    \"\"\"" << endl
          << "     conn should be  " << endl
          << "     params: " << strutil::JoinStrings(comment_params, ", ")
          <<                    endl
          << "     returns: " << functionReturn.name_ << endl
          << "    \"\"\"" << endl
          << "    __s = []" << endl
          << strutil::JoinStrings(preparation, "\n") << endl
          << "    reply = self.__conn.Request(self.__url_base," << endl
          <<"                                 self.__service_name," << endl
          << "                                '" << functionName << "', " << endl
          << "                                '%s' % ', '.join(__s))" << endl
          << "    self.__data = {}" << endl
          << "    self.set_" << retAttr.name_ <<  "(reply)" << endl
          << "    ret = self.__data.get('" << retAttr.name_ << "', None)" << endl
          << "    self.__data = {}" << endl
          << "    return ret" << endl
          << endl
          << endl;
    }
  }
  return true;
}
}
