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

#include "common/io/input_stream.h"
#include "common/io/buffer/io_memory_stream.h"
#include "net/rpc/lib/server/rpc_core_types.h"

namespace rpc {

rpc::Query::Query(const rpc::Transport& transport,
                  uint32 qid,
                  const string& service,
                  const string& method,
                  const io::MemoryStream& args,
                  const rpc::Codec& codec,
                  uint32 rid)
    : transport_(transport),
      codec_(codec.Clone()),
      qid_(qid),
      service_(service),
      method_(method),
      args_(),
      args_decoder_(codec_->CreateDecoder(args_)),
      args_decoding_initialized_(false),
      status_(static_cast<rpc::REPLY_STATUS>(-1)),
      result_(),
      result_encoder_(codec_->CreateEncoder(result_)),
      query_params_(NULL),
      completion_callback_(NULL),
      rid_(rid) {
  args_.AppendStreamNonDestructive(&args);
  // always keep a marker on stream start, to be able to rewind the stream
  args_.MarkerSet();
}

rpc::Query::~Query() {
  delete codec_;
  codec_ = NULL;
  delete args_decoder_;
  args_decoder_ = NULL;
  delete result_encoder_;
  result_encoder_ = NULL;
  delete query_params_;
  query_params_ = NULL;
}

io::MemoryStream& rpc::Query::RewindParams() {
  args_.MarkerRestore();
  args_.MarkerSet();
  args_decoding_initialized_ = false;
  return args_;
}
bool rpc::Query::InitDecodeParams() {
  CHECK ( !args_decoding_initialized_ );
  uint32 item_count;
  if ( DECODE_RESULT_SUCCESS != args_decoder_->DecodeArrayStart(item_count) ) {
    return false;
  }
  // initialized
  args_decoding_initialized_ = true;
  return true;
}
bool rpc::Query::HasMoreParams() {
  CHECK ( args_decoding_initialized_ );
  bool has_more_items;
  DECODE_RESULT result = args_decoder_->DecodeArrayContinue(has_more_items);
  return result == DECODE_RESULT_SUCCESS && has_more_items;
}

void rpc::Query::SetParams(rpc::Query::Params* params) {
  DCHECK(query_params_ == NULL);
  query_params_ = params;
}

void rpc::Query::SetCompletionCallback(
    Callback1<const rpc::Query&>* completion_callback) {
  CHECK_NULL(completion_callback_);
  completion_callback_ = completion_callback;
  CHECK_NOT_NULL(completion_callback_);
}

string rpc::Query::ToString() const {
  ostringstream oss;
  oss << "rpc::Query {"
      << " qid=" << qid_
      << " service=" << service_
      << " method=" << method_
      << " codec=" << codec_->ToString()
      << " args=" << args_.DebugString()
      << " status=" << status_ << "(" << rpc::ReplyStatusName(status_) << ")"
      << " result=" << result_.DebugString()
      << " rid=" << rid_
      << "}";
  return oss.str();
}
}
