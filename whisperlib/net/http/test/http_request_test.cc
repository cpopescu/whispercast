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

#include <stdio.h>
#include "common/base/types.h"
#include "common/base/log.h"
#include "common/base/system.h"
#include "common/base/gflags.h"

#include "net/http/http_request.h"
#include "common/io/file/file.h"
#include "common/io/file/file_input_stream.h"
#include "common/io/file/file_output_stream.h"
#include "common/io/buffer/memory_stream.h"

//////////////////////////////////////////////////////////////////////

DEFINE_bool(is_server,
            false,
            "If true, we parse the requests from the given files as "
            "server replies");
DEFINE_string(input_file,
              "",
              "Process a request from this file");
DEFINE_bool(require_success,
            false,
            "We require that all parsing should happen OK");

//////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  io::MemoryStream ms;
  ms.Write(io::FileInputStream::ReadFileOrDie(FLAGS_input_file.c_str()));

  http::RequestParser parser(FLAGS_input_file.c_str());
  parser.set_dlog_level(true);
  http::Request req;
  parser.Clear();
  if ( FLAGS_is_server ) {
    req.client_header()->PrepareRequestLine("/");
    const int ret = parser.ParseServerReply(&ms, &req);
    LOG_INFO << parser.name() << " Returned: "
             << http::RequestParser::ReadStateName(ret)
             << " [" << ret << "] "
             << " in " << parser.ParseStateName()
             << " with: " << ms.Size() << " left in the input buffer !!";
    if ( ret & http::RequestParser::REQUEST_FINISHED ) {
      CHECK(parser.InFinalState());
      if ( parser.InErrorState() ) {
        LOG_INFO << parser.name() << " ended in error: "
                 << parser.ParseStateName();
      } else {
        ms.Clear();
        req.AppendServerReply(&ms, false, false);
        LOG_INFO << " Output: \n" <<  ms.ToString();
      }
    } else {
      CHECK(!parser.InFinalState());
      LOG_INFO << parser.name() << " did not finish parsing w/ data in "
               << FLAGS_input_file;
    }
    printf("%d %d\n", static_cast<int>(ret), static_cast<int>(ms.Size()));
  } else {
    const int ret = parser.ParseClientRequest(&ms, &req);
    LOG_INFO << parser.name() << " Returned: "
             << http::RequestParser::ReadStateName(ret)
             << " [" << ret << "] "
             << " in " << parser.ParseStateName()
             << " with: " << ms.Size() << " left in the input buffer !!";
    if ( ret & http::RequestParser::REQUEST_FINISHED ) {
      CHECK(parser.InFinalState());
      if ( parser.InErrorState() ) {
        LOG_INFO << parser.name() << " ended in error: "
                 << parser.ParseStateName();
      } else {
        ms.Clear();
        req.AppendClientRequest(&ms);
        LOG_INFO << " Output: \n" <<  ms.ToString();
      }
    } else {
      CHECK(!parser.InFinalState());
      LOG_INFO << parser.name() << " did not finish parsing w/ data in "
               << FLAGS_input_file;
    }
    printf("%d %d\n", static_cast<int>(ret), static_cast<int>(ms.Size()));
  }
}
