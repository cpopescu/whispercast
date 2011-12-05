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

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/strutil.h>

#include <whisperlib/net/base/selector.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_decoder.h>
#include <whisperlib/common/io/file/file.h>
#include <whisperlib/common/io/file/file_output_stream.h>
#include <whisperlib/common/io/file/file_input_stream.h>

#include "elements/factory.h"

//////////////////////////////////////////////////////////////////////

DEFINE_string(outfile,
              "",
              "We test the saving of our last config to this file");
DEFINE_string(outfile2,
              "",
              "We test the saving of what we read in --infile to this file");
DEFINE_string(infile,
              "",
              "We test reading from this guy");

//////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  MediaElementSpecs s_command;
  s_command.name_.Ref() = "command_perm";
  s_command.command_spec_.Ref().media_type_.Ref()  = "flv";
  s_command.command_spec_.Ref().commands_.Ref().PushBack(
    CommandSpec("src1", "cmd1",  true));
  s_command.command_spec_.Ref().commands_.Ref().PushBack(
    CommandSpec("src2", "cmd2",  false));
  MediaElementSpecs s_http;
  s_http.name_.Ref() = "http_temp";
  s_http.http_client_spec_.Ref().http_data_.Ref().PushBack(
    HttpClientElementDataSpec("http1", "192.168.2.1", 8080,
                       "/stream1", false, "flv"));
  s_http.http_client_spec_.Ref().http_data_.Ref().PushBack(
    HttpClientElementDataSpec("http2", "192.168.2.2", 8081,
                       "/stream2", false, "flv"));
  MediaElementSpecs s_file;
  s_file.name_.Ref() = "file_perm";
  s_file.file_spec_.Ref() = FileElementSpec("mp3", "/tmp", ".*\\.mp3", false);
  MediaElementSpecs s_switching_spec;
  s_switching_spec.name_.Ref() = "switching_perm";
  s_switching_spec.switching_spec_.Ref() = SwitchingElementSpec(
      "random_policy", "flv");

  ElementConfigurationSpecs specs;
  specs.permanent_elements_.Ref().PushBack(s_switching_spec);
  specs.permanent_elements_.Ref().PushBack(s_command);
  specs.permanent_elements_.Ref().PushBack(s_file);
  specs.temp_elements_.Ref().PushBack(s_http);

  streaming::ElementFactory factory(NULL, NULL, NULL, NULL, NULL, NULL, "/");
  vector<streaming::ElementFactory::ErrorData> errors;
  CHECK(!factory.AddSpecs(specs, &errors));

  // Add the missing policy
  PolicySpecs policy_random;
  policy_random.name_.Ref() = "random_policy";
  policy_random.random_policy_.Ref().max_history_size_.Ref() = 10;
  specs.policies_.Ref().PushBack(policy_random);


  // Should fail on duplicated specs
  CHECK(!factory.AddSpecs(specs, &errors));

  factory.Clear();
  errors.clear();
  CHECK(factory.AddSpecs(specs, &errors));

  streaming::ElementFactory::ElementMap elements;
  streaming::ElementFactory::PolicyMap policies;

  CHECK(!factory.CreateElement("ahahah", NULL, NULL,
                                &elements, &policies));
  CHECK(factory.CreateElement("switching_perm", NULL, NULL,
                              &elements, &policies));
  CHECK_EQ(elements.size(), 1);
  CHECK_EQ(policies.size(), 1);

  streaming::ElementFactory::ErrorData error;
  MediaElementSpecs* s_command2 = new MediaElementSpecs(s_command);
  PolicySpecs* policy_random2 = new PolicySpecs(policy_random);
  CHECK(!factory.AddElementSpec(s_command2, true, &error));
  CHECK(!factory.AddElementSpec(s_command2, false, &error));

  CHECK(!factory.DeleteElementSpec("inexistent", &error));
  CHECK(!factory.DeletePolicySpec("random_policy", &error));
  CHECK(!factory.AddPolicySpec(policy_random2, &error));
  CHECK(!factory.ChangePolicy("switching_perm", "random2", &error));
  CHECK(!factory.DeletePolicySpec("random2", &error)) << error.ToString();

  // Now these should not..
  s_command2->name_.Ref() = "command_perm2";
  policy_random2->name_.Ref() = "random2";

  CHECK(!factory.DeletePolicySpec("random2", &error)) << error.ToString();
  CHECK(factory.AddElementSpec(s_command2, true, &error));
  CHECK(factory.AddPolicySpec(policy_random2, &error));

  CHECK(factory.ChangePolicy("switching_perm", "random2", &error));
  CHECK(factory.DeletePolicySpec("random_policy", &error));


  rpc::Map<rpc::Int, rpc::String>  gugu;
  gugu.Insert(rpc::Int(132), rpc::String("abc"));
  gugu.Insert(rpc::Int(328), rpc::String("abc4"));
  gugu.Insert(rpc::Int(9028), rpc::String("abc2"));

  if ( !FLAGS_outfile.empty() ) {
    io::File* const outfile = io::File::CreateFileOrDie(FLAGS_outfile.c_str());
    io::FileOutputStream fos(outfile);
    rpc::JsonEncoder encoder(fos);
    encoder.Encode(gugu);
  }
  if ( !FLAGS_infile.empty() ) {
    io::File* const infile = io::File::OpenFileOrDie(FLAGS_infile.c_str());
    io::FileInputStream fis(infile);
    rpc::JsonDecoder decoder(fis);
    ElementConfigurationSpecs specs2;
    LOG_INFO << " Decoder: "  << decoder.Decode(specs2);

    if ( !FLAGS_outfile2.empty() ) {
      io::File* const outfile =
        io::File::CreateFileOrDie(FLAGS_outfile2.c_str());
      io::FileOutputStream fos(outfile);
      rpc::JsonEncoder encoder(fos);
      encoder.Encode(specs2);
    }
  }
  printf("PASS\n");
}
