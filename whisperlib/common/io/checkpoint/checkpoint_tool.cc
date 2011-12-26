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

#include <whisperlib/common/io/checkpoint/checkpointing.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_decoder.h>

//////////////////////////////////////////////////////////////////////

DEFINE_string(in,
              "",
              "Read checkpoint from this file.");

DEFINE_string(out,
              "",
              "Write checkpoint (with imports) into this file.");

DEFINE_bool(print,
            false,
            "Print the content of the checkpoint");

DEFINE_string(import_json_file,
              "",
              "Add to current checkpoint the given JSON (string->string) map."
              "You probably want to use -out flag too.");

DEFINE_string(export_json_file,
              "",
              "Export the whole checkpoint as a JSON (string->string) map"
              " into this file.");

//////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  //////////////////////////////////////////////////////////////////////////

  map<string, string> checkpoint;

  // maybe read input checkpoint
  if ( FLAGS_in != "" ) {
    if ( !io::ReadCheckpointFile(FLAGS_in, &checkpoint) ) {
      LOG_ERROR << "Failed to read checkpoint file: [" << FLAGS_in << "]";
      common::Exit(1);
    }
    LOG_INFO << "Successfully read checkpoint file: [" << FLAGS_in << "]";
  }

  // maybe import a JSON file
  if ( FLAGS_import_json_file != "" ) {
    io::File json_file;
    if ( !json_file.Open(FLAGS_import_json_file,
                         io::File::GENERIC_READ,
                         io::File::OPEN_EXISTING) ) {
      LOG_ERROR << "Cannot open file: [" << FLAGS_import_json_file << "]";
      common::Exit(1);
    }
    io::MemoryStream ms;
    json_file.Read(&ms, kMaxInt32);
    string s;
    ms.ReadString(&s);
    if ( !rpc::JsonDecoder::DecodeObject(s, &checkpoint) ) {
      LOG_ERROR << "Failed to decode json import file: ["
                << FLAGS_import_json_file << "]";
      common::Exit(1);
    }
    LOG_INFO << "Successfully import json file: ["
             << FLAGS_import_json_file << "]";
  }

  // maybe print the checkpoint
  if ( FLAGS_print ) {
    for ( map<string, string>::const_iterator it = checkpoint.begin();
          it != checkpoint.end(); ++it ) {
      LOG_INFO << "Name: [" << it->first << "]";
      LOG_INFO << "Value: [" << it->second << "]";
    }
  }

  // maybe export to JSON file
  if ( FLAGS_export_json_file != "" ) {
    io::File json_file;
    if ( !json_file.Open(FLAGS_export_json_file,
                         io::File::GENERIC_READ_WRITE,
                         io::File::CREATE_ALWAYS) ) {
      LOG_ERROR << "Cannot open file: [" << FLAGS_export_json_file << "]";
      common::Exit(1);
    }
    json_file.Write(rpc::JsonEncoder::EncodeObject(checkpoint));
    LOG_INFO << "Export json: [" << json_file.filename() << "]";
    json_file.Close();
  }

  // maybe write checkpoint to file
  if ( FLAGS_out != "" ) {
    if ( !io::WriteCheckpointFile(checkpoint, FLAGS_out) ) {
      LOG_ERROR << "Failed to WriteCheckpointFile, to: [" << FLAGS_out << "]";
      common::Exit(1);
    }
  }

  LOG_INFO << "Done #" << checkpoint.size() << " pairs";

  common::Exit(0);
}
