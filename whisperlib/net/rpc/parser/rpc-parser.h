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

#ifndef __NET_RPC_PARSER_RPC_PARSER_H__
#define __NET_RPC_PARSER_RPC_PARSER_H__

#include <stdarg.h>

#include <string>
#include <vector>
#include <list>
#include <iostream>

#include <whisperlib/common/base/types.h>
#include <whisperlib/net/rpc/parser/rpc-ptypes.h>

// When we're included from rpc-parser.yy the yyFlexLexer class is already
// defined and including <FlexLexer.h> will double-define yyFlexLexer.

// When we're included by some other user file yyFlexLexer needs to be defined.

// defined in rpc-parser.yy
#ifndef RPC_PARSER_YY_INCLUDED
#  include <FlexLexer.h>
#endif

namespace rpc {

class Parser : public yyFlexLexer {
 protected:
  // list of all custom types declared in .rpc files
  // Filled during yylex() run with every new custom-type found.
  PCustomTypeArray& customTypes_;

  // list of all services declared in .rpc files.
  // Filled during yylex() run with every new service found.
  PServiceArray& services_;

  // list of verbatim texts declared in .rpc files
  PVerbatimArray& verbatim_;

  // name of the current file being parsed
  string strFilename_;

  // true if at least one error was found on parse.
  // false otherwise.
  bool bErrorsFound_;

  // true if you want PRINT_INFO logs to be displayed
  bool bPrintInfo_;

 public:
  Parser(std::istream& input,
         std::ostream& output,
         PCustomTypeArray& customTypes,
         PServiceArray& services,
         PVerbatimArray& verbatim,
         const char* filename,
         bool bPrintInfo);
  virtual ~Parser();

  bool Run();

 public:
  // Auto-generated parser routine.
  int yylex();
};
}

#endif  // __NET_RPC_PARSER_RPC_PARSER_H__
