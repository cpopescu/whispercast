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

#include <list>
#include <string>
#include <stdio.h>
#include <stdarg.h>

#include "common/base/types.h"
#include "common/base/errno.h"
#include "common/base/log.h"
#include "common/base/gflags.h"
#include "common/base/scoped_ptr.h"
#include "net/rpc/lib/types/rpc_all_types.h"
#include "net/rpc/lib/client/rpc_client_connection_tcp.h"
#include "net/rpc/lib/codec/rpc_codec_factory.h"
#include "net/rpc/lib/codec/binary/rpc_binary_codec.h"
#include "net/rpc/lib/codec/json/rpc_json_codec.h"

#include "test.h"

DEFINE_string(server,
             "127.0.0.1:5611",
             "The address of the RPC server.");

#define LOG_TEST LOG_INFO

int main(int argc, char** argv) {
  // - Initialize logger.
  // - Install default signal handlers.
  common::Init(argc, argv);

  const net::HostPort serverAddress(FLAGS_server.c_str());
  if ( serverAddress.IsInvalid() ) {
    LOG_ERROR << "Invalid server address: [" << FLAGS_server << "]";
    common::Exit(0);
    return 0;
  }

  {
    // create selector
    //
    net::SelectorThread selectorThread;
    net::Selector* selector = selectorThread.mutable_selector();
    selectorThread.Start();

    // create net factory
    //
    net::NetFactory net_factory(selector);

    // run test for every codec
    //
    rpc::CODEC_ID codec_ids [] = {rpc::CID_BINARY,
                                  rpc::CID_JSON
                               };
    for ( int i = 0; i < sizeof(codec_ids) / sizeof(codec_ids[0]); ++i ) {
      const rpc::CODEC_ID codec_id = codec_ids[i];
      scoped_ptr<rpc::Codec> codec(rpc::CodecFactory::Create(codec_id));
      LOG_WARNING << "Using " << codec->GetCodecName() << " codec.";

      // create RPC connection
      //
      rpc::ClientConnectionTCP rpcConnection(
          *selector, net_factory, net::PROTOCOL_TCP,
          serverAddress, codec->GetCodecID());
      if ( !rpcConnection.Open() ) {
        LOG_ERROR << "Failed to connect to RPC server: " << serverAddress;
        break;
      }

      RunTest(rpcConnection);
    }
  }
  LOG_TEST << "Bye.";
  common::Exit(0);
  return 0;
}
