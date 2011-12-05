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

#include <string.h>

#include <fstream>
#include <map>

#include "net/rpc/parser/rpc-ptypes.h"
#include "net/rpc/parser/rpc-ptypes-check.h"
#include "net/rpc/parser/rpc-parser.h"
#include "net/rpc/parser/export/rpc-exporter.h"
#include "net/rpc/parser/export/C++/rpc-exporter-cc.h"
#include "net/rpc/parser/export/Javascript/rpc-exporter-js.h"
#include "net/rpc/parser/export/Python/rpc-exporter-python.h"
#include "net/rpc/parser/export/Php/rpc-exporter-php.h"

#include "common/base/types.h"
#include "common/base/scoped_ptr.h"
#include "common/base/log.h"
#include "common/base/gflags.h"
#include "common/base/common.h"
#include "common/base/system.h"
#include "common/base/strutil.h"

//////////////////////////////////////////////////////////////////////

DEFINE_string(cc,
              "",
              "Parameters for the C++ exporter "
              "(use --helpparams for details)");
DEFINE_string(js,
              "",
              "Parameters for the Javascript exported "
              "(use --helpparams for details)");
DEFINE_string(php,
              "",
              "Parameters for the PHP exporter "
              "(use --helpparams for details)");
DEFINE_string(py,
              "",
              "Parameters for the Python exporter  "
              "(use --helpparams for details)");
DEFINE_bool(verbose,
            false,
            "We are verbose in our error messages");
DEFINE_bool(help_params,
            false,
            "Details the possible params for exporters");

//////////////////////////////////////////////////////////////////////

typedef map<string, string> MapStringString;
typedef vector<string> ArrayString;

bool CliInsertNonDuplicate(
  const char* name,
  const char* value,
  MapStringString& out) {
  CHECK_NOT_NULL(name);

  // insert empty string instead of NULL
  //
  if ( value == NULL ) {
    value = "";
  }

  // try to insert into map
  //
  pair<MapStringString::iterator, bool> result =
    out.insert(MapStringString::value_type(name, value));

  // check if we already have this parameter in map
  //
  if ( result.second == false ) {
    // key already exists and was not replaced
    //
    const string& prev_value = result.first->second;
    LOG_ERROR << "ERROR: duplicate parameter " << name << endl
              << " first value: \"" << prev_value << "\"" << endl
              << " second value: \"" << value << "\"";
    return false;
  }

  return true;
}

//
//  Parse command line arguments
// input:
//   argc: number of command line arguments. As received by main(..).
//   argv: arguments vector. As received by main(..).
//   inputFiles: fill with input file names
//   flags: fill with simple parameters. e.g. "-v"
//   params: fill with complex parameters. e.g. "--c out=abc.h"
// returns:
//  success value.
//
bool CliParseCommandLine(ArrayString& flags,
                         MapStringString& params) {
  if ( !FLAGS_cc.empty() ) {
    CliInsertNonDuplicate("cc", FLAGS_cc.c_str(), params);
  }
  if ( !FLAGS_js.empty() ) {
    CliInsertNonDuplicate("js", FLAGS_js.c_str(), params);
  }
  if ( !FLAGS_php.empty() ) {
    CliInsertNonDuplicate("php", FLAGS_php.c_str(), params);
  }
  if ( !FLAGS_py.empty() ) {
    CliInsertNonDuplicate("py", FLAGS_py.c_str(), params);
  }
  return true;
}

bool CliParseExportParams(const char* szExporterParams,
                          MapStringString& out) {
  string strExporterParams(szExporterParams);
  strutil::StrTrimChars(strExporterParams, " '\"");

  char* const szTmp = strdup(strExporterParams.c_str());

  const char* kDelim = ",;";
  char* pch = strtok(szTmp, kDelim);
  while ( pch != NULL ) {
    char* pname = pch;
    char* pvalue = strchr(pch, '=');
    if ( pvalue ) {
      *(pvalue++) = 0;
    }
    const string strName(strutil::StrTrimChars(string(pname), " '\""));
    const string strValue(pvalue
                          ? strutil::StrTrimChars(string(pvalue), " '\"")
                          : string());
    if ( strValue.empty() ) {
      const string strName(strutil::StrTrimChars(string(pname), " '\""));
      LOG_ERROR << "Missing value for export parameter \"" << strName << "\"";
      free(szTmp);
      return false;
    }
    LOG_INFO << "Export param extracted: ["
             << strName << ":" << strValue << "]";
    if ( !CliInsertNonDuplicate(strName.c_str(), strValue.c_str(), out) ) {
      LOG_ERROR << " Error inserting param !";
      free(szTmp);
      return false;
    }
    pch = strtok(NULL, kDelim);
  }
  free(szTmp);
  return true;
}

int main(int argc, char** argv) {
  ArrayString inputFiles;
  for ( int i = 1; i < argc; ++i ) {
    if ( *argv[i]  != '-' ) {
      inputFiles.push_back(argv[i]);
    }
  }

  common::Init(argc, argv);

  LOG_INFO << " FLAG: cc[" << FLAGS_cc << "]";
  if ( FLAGS_help_params ) {
    // print usage and return
    //
    cout << "Usage: " << argv[0]
         << " <input rpc file1> <input rpc file2>... "
         << " --cc=<C++ params>"
            " --js=<Javascript params>"
            " --php<PHP params>"
            " --py<Python params>" << endl;
    cout << "Where: all language params are contained in 1 single "
        "string with the following format: 'param=value, param=value, ...'. "
        "All params are optional, unless specified otherwise. "
        "Not specifying a certain output file will not generate "
        "that file." << endl;
    cout << "C++ params:" << endl;
    {
      const map<string, string>& paramsDesc =
          rpc::ExporterCC::ParamsDescription();
      for ( map<string, string>::const_iterator it = paramsDesc.begin();
            it != paramsDesc.end(); ++it ) {
        const string& pname = it->first;
        const string& pdesc = it->second;
        cout << "  " << pname << " = <filename> " << pdesc << endl;
      }
    }
    cout << "Javascript params:" << endl;
    {
      const map<string, string>& paramsDesc =
          rpc::ExporterJS::ParamsDescription();
      for ( map<string, string>::const_iterator it = paramsDesc.begin();
            it != paramsDesc.end(); ++it ) {
        const string& pname = it->first;
        const string& pdesc = it->second;
        cout << "  " << pname << " = <filename> " << pdesc << endl;
      }
    }
    cout << "PHP params:" << endl;
    {
      const map<string, string>& paramsDesc =
          rpc::ExporterPHP::ParamsDescription();
      for ( map<string, string>::const_iterator it = paramsDesc.begin();
            it != paramsDesc.end(); ++it ) {
        const string& pname = it->first;
        const string& pdesc = it->second;
        cout << "  " << pname << " = <filename> " << pdesc << endl;
      }
    }
    cout << "Python params:" << endl;
    {
      const map<string, string>& paramsDesc =
          rpc::ExporterPython::ParamsDescription();
      for ( map<string, string>::const_iterator it = paramsDesc.begin();
            it != paramsDesc.end(); ++it ) {
        const string& pname = it->first;
        const string& pdesc = it->second;
        cout << "  " << pname << " = <filename> " << pdesc << endl;
      }
    }
    return 0;
  }

  ArrayString flags;
  MapStringString cmdParams;

  CHECK(CliParseCommandLine(flags, cmdParams));

  // make absolute paths for all inputFiles
  //
  // [COSMIN] But for log sake, leave them relative
  // for ( ArrayString::iterator it = inputFiles.begin();
  //      it != inputFiles.end(); ++it) {
  //   string& filename = *it;
  //   filename = StringUtils::MakeAbsolutePath(filename.c_str());
  // }

  // analyze command line parameters. Ignore exporters for now.
  //
  for ( MapStringString::iterator it = cmdParams.begin();
        it != cmdParams.end(); ++it ) {
    const string& name  = it->first;
    const string& value = it->second;
    CHECK(!value.empty());

    LOG_INFO << "CLI Parameter: " << name << " = " << value;

    // ignore exporters in this stage
    //
    if ( name == "cc" ) { continue; }
    if ( name == "js" ) { continue; }
    if ( name == "php" ) { continue; }
    if ( name == "py" ) { continue; }

    LOG_ERROR << "Unrecognized parameter: " << name;
    return -1;
  }

  if ( inputFiles.empty() ) {
    LOG_ERROR << "No input files specified.";
    return -1;
  }

  // create the lists of customTypes and services.
  // These will be populated by the parser.
  //
  PCustomTypeArray customTypes;
  PServiceArray services;
  PVerbatimArray verbatim;
  for ( ArrayString::iterator it = inputFiles.begin();
        it != inputFiles.end(); ++it ) {
    const char* inputFile = it->c_str();
    LOG_INFO << "Parsing file: " << inputFile;

    // open the input file as istream
    //
    ifstream file(inputFile);
    if ( !file.is_open() ) {
      LOG_ERROR << "Failed to open file: [" << inputFile << "]";
      return -1;
    }

    // parse input file and fill in those lists.
    // TODO(cosmin): The input file may import additional files,
    //               the parser should handle those too.
    //
    rpc::Parser lexer(file, cout, customTypes, services, verbatim, inputFile,
                      FLAGS_verbose);
    CHECK(lexer.Run()) << " Parser error in file " << inputFile;
  }

  // LOG the customTypes& services found
  //
  LOG_INFO << "Number of CustomTypes found: " << customTypes.size();
  for ( PCustomTypeArray::const_iterator it = customTypes.begin();
        it != customTypes.end(); ++it ) {
    const PCustomType& customType = *it;
    LOG_INFO << "  - " << customType.name_.c_str() << ":";

    const PCustomType::AttributeArray& attributes = customType.attributes_;
    for ( PCustomType::AttributeArray::const_iterator it = attributes.begin();
          it != attributes.end(); ++it ) {
      const PCustomType::Attribute& attribute = *it;
      LOG_INFO << "    - " << (attribute.isOptional_ ? "OPTIONAL" : "REQUIRED")
               << " " << attribute.type_ << " " << attribute.name_;
    }
  }
  LOG_INFO << "Number of services found: " << services.size();
  for ( PServiceArray::const_iterator it = services.begin();
        it != services.end(); ++it ) {
    const PService& service = *it;
    LOG_INFO << "  - " << service.name_ << ": ";

    const PFunctionArray& functions = service.functions_;
    for ( PFunctionArray::const_iterator it = functions.begin();
          it != functions.end(); ++it ) {
      const PFunction& func = *it;
      string s = "    - " + func.name_ + " (";

      const PParamArray& args = func.input_;
      for ( PParamArray::const_iterator it = args.begin();
            it != args.end(); ) {
        const PParam& arg = *it;
        s += arg.type_.ToString() + " " + arg.name_;

        ++it;
        if ( it != args.end() ) {
          s += ", ";
        }
      }
      s += ") => " + func.output_.ToString();
      LOG_INFO << s;
    }
  }

  // create the list of exporters
  //
  typedef list<rpc::Exporter*> ExporterList;
  ExporterList exporters;

  for ( MapStringString::iterator it = cmdParams.begin();
        it != cmdParams.end(); ++it ) {
    const string& name  = it->first;
    const string& value = it->second;

#define CONSIDER_EXPORTER(opName, exporterName)                         \
    if ( name == opName ) {                                             \
      /* parse export parameters and pass these to the exporter */      \
      MapStringString exportParams;                                     \
      if ( !CliParseExportParams(value.c_str(), exportParams) ) {       \
        LOG_ERROR << " Error in CliParseExportParams";                  \
        return -1;                                                      \
      }                                                                 \
      LOG_INFO << opName << " export params: "                          \
               << strutil::ToString(exportParams);                      \
      rpc::Exporter* exporter = new exporterName(customTypes,           \
                                                 services,              \
                                                 verbatim,              \
                                                 inputFiles);           \
      if ( !exporter->SetParams(exportParams) ) {                       \
        LOG_ERROR << " Error in exporter->SetParams";                   \
        return -1;                                                      \
      }                                                                 \
      exporters.push_back(exporter);                                    \
      continue;                                                         \
    }

    CONSIDER_EXPORTER("cc", rpc::ExporterCC);
    CONSIDER_EXPORTER("js", rpc::ExporterJS);
    CONSIDER_EXPORTER("php", rpc::ExporterPHP);
    CONSIDER_EXPORTER("py", rpc::ExporterPython);
#undef CONSIDER_EXPORTER

    // ignore other parameters
  }

  // verify types definitions correctness
  //
  if ( !VerifyPTypesCorrectenss(customTypes, services,
                                rpc::ExporterCC::RestrictedKeywords()) ||
       !VerifyPTypesCorrectenss(customTypes, services,
                                rpc::ExporterJS::RestrictedKeywords()) ||
       !VerifyPTypesCorrectenss(customTypes, services,
                                rpc::ExporterPHP::RestrictedKeywords()) ||
       !VerifyPTypesCorrectenss(customTypes, services,
                                rpc::ExporterPython::RestrictedKeywords()) ) {

    LOG_ERROR << "Error on VerifyPTypesCorrectenss !!";
    return -1;
  }

  if(exporters.empty()) {
    LOG_ERROR << "No translator specified.";
    return -1;
  }

  // call the exporters on the generated types & services
  //
  for ( ExporterList::iterator it = exporters.begin();
        it != exporters.end(); ++it ) {
    rpc::Exporter& exporter = **it;

    // execute the translation of the customTypes and services into the
    // specified language
    //
    bool success = exporter.Export();
    if ( !success ) {
      LOG_ERROR << "Failed to translate rpc customTypes and services for"
                   " language: " << exporter.GetLanguageName();
      return -1;
    }
  }

  // delete exporters
  //
  while ( !exporters.empty() ) {
    rpc::Exporter* exporter = exporters.front();
    exporters.pop_front();
    delete exporter;
  }

  return 0;
}
