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


#ifndef __NET_RPC_LIB_SERVER_RPC_CORE_TYPES_H__
#define __NET_RPC_LIB_SERVER_RPC_CORE_TYPES_H__

#include <vector>
#include <string>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/callback.h>

#include <whisperlib/common/io/input_stream.h>
#include <whisperlib/common/io/output_stream.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/common/io/buffer/io_memory_stream.h>

#include <whisperlib/net/base/address.h>
#include <whisperlib/net/rpc/lib/codec/rpc_encoder.h>
#include <whisperlib/net/rpc/lib/codec/rpc_decoder.h>
#include <whisperlib/net/rpc/lib/codec/rpc_codec.h>
#include <whisperlib/net/rpc/lib/types/rpc_all_types.h>
#include <whisperlib/net/rpc/lib/types/rpc_message.h>

// Types to be used inside server for RPC queries in transit.
// rpc::Transport: is just a container describing the transport layer for RPC.
// rpc::Query: represents a remote call, on the server side. It is built by
//           the transport layer (rpc::ServerConnection) and then circulates
//           through the execution layer until completed.
//           Initially contains the service name, the method name,
//           and the encoded arguments; but during execution it receives the
//           decoded call parameters to keep them alive while the call is
//           in execution.

namespace rpc {

class Transport {
 public:
  enum Protocol {
    TCP,
    HTTP,
  };
  explicit Transport(const Transport& transport)
    : protocol_(transport.protocol()),
      local_address_(transport.local_address()),
      peer_address_(transport.peer_address()),
      user_(transport.user()),
      passwd_(transport.passwd()) {
  }

  Transport(Protocol protocol,
            const net::HostPort& local_address,
            const net::HostPort& peer_address)
      : protocol_(protocol),
        local_address_(local_address),
        peer_address_(peer_address) {
  }
  virtual ~Transport() {
  }

  Protocol protocol() const                  { return protocol_;       }
  const net::HostPort& local_address() const { return local_address_;  }
  const net::HostPort& peer_address() const  { return peer_address_;   }
  const string& user() const                 { return user_;           }
  const string& passwd() const               { return passwd_;         }

  void set_user_passwd(const string& user,
                       const string& passwd) { user_ = user;
                                               passwd_ = passwd;       }

 protected:
  const Protocol protocol_;
  const net::HostPort local_address_;
  const net::HostPort peer_address_;
  string user_;
  string passwd_;
};

class Query {
 public:
  // input:
  //  - qid: a value to be passed back in result handling, usefull for mapping
  //         pairs [qid, rpc::Query]
  //  - service: service name.
  //  - method: method name (inside service).
  //  - args: contains the query arguments encoded. And that's all it contains.
  //  - args_dec: inside 'args' the arguments should be decoded by this decoder.
  //  - result_enc: the result will be encoded using this encoder
  //  - rid: result handler ID. Used by the IAsyncQueryExecutor to find the
  //         result handler of this query.
  Query(const rpc::Transport& transport,
        uint32 qid,
        const string& service,
        const string& method,
        const io::MemoryStream& args,
        const CodecId codec,
        uint32 rid);
  virtual ~Query();

  const rpc::Transport& transport() const { return transport_;  }
  uint32 qid() const                      { return qid_; }
  const string& service() const           { return service_; }
  const string& method() const            { return method_; }
  rpc::REPLY_STATUS status() const        { return status_; }
  const io::MemoryStream& result() const  { return result_; }
  CodecId codec() const                   { return codec_; }
  uint32 rid() const                      { return rid_; }
  io::MemoryStream& result()              { return result_; }

  //////////////////////////////////////////////////////////////////////
  //
  //       Decoding arguments
  //
  io::MemoryStream& RewindParams();

  // MUST be called before DecodeParam or HasMoreParams.
  // returns success status.
  bool InitDecodeParams();

  template <typename T>
  bool DecodeParam(T& obj) {
    CHECK ( args_decoding_initialized_ );
    bool has_more_attribs = false;
    string argName;
    return (DECODE_RESULT_SUCCESS ==
              args_decoder_->DecodeArrayContinue(args_, &has_more_attribs) &&
            has_more_attribs &&
            DECODE_RESULT_SUCCESS == args_decoder_->Decode(args_, &obj));
  }

  // returns:
  //  true -> more params available to DecodeParam
  //  false -> reached parameter's array end.
  bool HasMoreParams();

  //////////////////////////////////////////////////////////////////////
  //
  // Encoding the result and completing the call
  //

  // Store decoded arguments inside query, so they are valid as long as the
  // query is in execution.
  class Params {
   public:
    Params() { }
    virtual ~Params() { }
  };
  void SetParams(rpc::Query::Params* params);

  // Set by the execution layer.
  // The completion_callback is usually a function inside the execution layer.
  void SetCompletionCallback(
      Callback1<const rpc::Query &>* completion_callback);

  // Called by the service implementation to return query result back to the
  // execution layer.
  template <typename T>
  void Complete(rpc::REPLY_STATUS status, const T& result) {
    result_.Clear();
    status_ = status;
    EncodeBy(codec_, result, &result_);
    CHECK_NOT_NULL(completion_callback_) << "CompletionCallback not set!";
    completion_callback_->Run(*this);
    delete this;   // This is bad, other solutions are welcomed.

    // Alternative: let the transport layer delete the query.
    // But keep in mind that we're calling the transport from this context,
    // so the call will return here to find a deleted query.
  }
  template <typename T>
  void Complete(const T& result) {
    Complete(RPC_SUCCESS, result);
  }
  void Complete(rpc::REPLY_STATUS status = RPC_SUCCESS) {
    Complete(status, rpc::Void());
  }

  string ToString() const;

 protected:
  // informations from the RPC trasport layer (like protocol, IP, port ..)
  const rpc::Transport transport_;

  // query identifier, used by the transport layer
  const uint32 qid_;
  // rpc service name
  const string service_;
  // rpc method name
  const string method_;
  // args_ and result_ are encoded by this codec
  CodecId codec_;
  // contains the query arguments encoded according to 'codec_'
  io::MemoryStream args_;
  // Because args are decoded in multiple calls, this persistent decoder is
  // needed. Locally allocated.
  rpc::Decoder* args_decoder_;
  // false = the args_ stream position is on the first arg (== stream begining)
  bool args_decoding_initialized_;

  // receives the return status
  rpc::REPLY_STATUS status_;
  // receives the return value, encoded according to codec_
  io::MemoryStream result_;

  // execution

  // Contains the call arguments keeping them valid while the query is in
  // executio, so you don't need to duplicate the arguments if you want to
  // delay query completion.
  Params* query_params_;

  // Set by the execution layer.
  // Used for passing the completed query's result back to the execution layer.
  Callback1<const rpc::Query&>* completion_callback_;

  // ID of an IResultHandler register in the execution layer who will
  // receive the result
  uint32 rid_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(Query);
};

// The rpc::CallContext is used in service implementation to access
// query's transport parameters and the Complete method.
// The service implementation should not see the entire rpc::Query.

template <typename T>
class CallContext {
 public:
  explicit CallContext(rpc::Query* query)
      : query_(query) {
  }
  virtual ~CallContext() {
  }

  const rpc::Transport& Transport() const {
    return query_->transport();
  }
  void Complete(rpc::REPLY_STATUS status) {
    query_->Complete(status);
    delete this;       // bad, but no other solution
  }
  void Complete(const T& result) {
    query_->Complete(result);
    delete this;        // bad, but no other solution
  }
  void Complete() {
    query_->Complete();
    delete this;        // bad, but no other solution
  }
 private:
  rpc::Query* const query_;
  DISALLOW_EVIL_CONSTRUCTORS(CallContext);
};
}

#endif  // __NET_RPC_LIB_SERVER_RPC_CORE_TYPES_H__
