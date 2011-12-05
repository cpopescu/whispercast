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

#include "net/http/http_proxy.h"

#define LOG_PROXY LOG_INFO_IF(dlog_level_) <<  ": "

namespace http {

Proxy::Proxy(net::Selector* selector,
             const net::NetFactory& net_factory,
             net::PROTOCOL net_protocol,
             http::Server* server,
             bool dlog_level)
  : selector_(selector),
    net_factory_(net_factory),
    net_protocol_(net_protocol),
    server_(server),
    dlog_level_(dlog_level) {
}

Proxy::~Proxy() {
  while ( !registered_paths_.empty() ) {
    server_->UnregisterProcessor(*registered_paths_.begin());
    registered_paths_.erase(registered_paths_.begin());
  }
  for ( ReqSet::const_iterator it = pending_requests_.begin();
        it != pending_requests_.end(); ++it ) {
    FinalizeRequest(*it);
  }
  pending_requests_.clear();
}

void Proxy::RegisterPath(const string& path,
                         net::HostPort remote_server,
                         const ClientParams* params) {
  http::Server::ServerCallback* callback = NewPermanentCallback(
    this, &Proxy::ProcessRequest, remote_server, params);
  server_->RegisterProcessor(path, callback, true, true);
  registered_paths_.insert(path);
}

void Proxy::UnregisterPath(const string& path) {
  server_->UnregisterProcessor(path);
  registered_paths_.erase(path);
}

void Proxy::ProcessRequest(net::HostPort remote_server,
                           const ClientParams* params,
                           ServerRequest* sreq) {
  ProxyReqStruct* preq = new ProxyReqStruct();
  preq->creq_ = new http::ClientRequest(
      sreq->request()->client_header()->method(),
      sreq->request()->url());
  preq->creq_->request()->client_header()->Clear();
  preq->creq_->request()->client_header()->CopyHeaders(
    *(sreq->request()->client_header()), true);
  preq->creq_->request()->client_header()->AddField(
    "X-Forwarded-For", sreq->remote_address().ip_object().ToString(), true);

  preq->creq_->request()->client_data()->AppendStream(
    sreq->request()->client_data());
  preq->creq_->set_is_pure_dumping(true);

  preq->proto_ = new ClientStreamReceiverProtocol(params,
      new http::SimpleClientConnection(selector_, net_factory_, net_protocol_),
      remote_server);

  preq->sreq_ = sreq;
  preq->callback_ = NewPermanentCallback(this, &Proxy::StreamingCallback, preq);
  pending_requests_.insert(preq);
  LOG_PROXY << " PROXY: Starting request for: " << *sreq->request()->url()
            << " [" << preq << "].. Header: \n"
            << preq->creq_->request()->client_header()->ToString();

  selector_->RunInSelectLoop(
    NewCallback(preq->proto_,
                &ClientStreamReceiverProtocol::BeginStreamReceiving,
                preq->creq_, preq->callback_));
}

void Proxy::StreamingCallback(ProxyReqStruct* preq) {
  if ( !selector_->IsInSelectThread() ) {
    selector_->RunInSelectLoop(NewCallback(this,
                                           &Proxy::StreamingCallback, preq));
    return;
  }
  if ( preq->creq_->is_finalized() &&
       preq->creq_->request()->server_data()->IsEmpty() ) {
    FinalizeRequest(preq);
    return;
  }
  int32 size_to_send = 0;
  size_to_send = preq->sreq_->free_output_bytes();
  if ( preq->sreq_->is_orphaned() ) {
    LOG_PROXY << " PROXY: orphaned request !";
    ClearRequest(preq);
    return;
  }
  if ( !preq->http_header_sent_ ) {
    preq->http_header_sent_ = true;
    preq->sreq_->request()->server_header()->CopyHeaders(
        *(preq->creq_->request()->server_header()), true);
    // These will be set from the request sending part ..
    preq->sreq_->request()->server_header()->SetContentEncoding(NULL);
    const bool try_chunked =
        preq->sreq_->request()->client_header()->http_version() >= VERSION_1_1;
    if ( try_chunked ) {
      preq->sreq_->request()->server_header()->ClearField(
          kHeaderContentLength);
    }
    // We clear this anyway ..
    preq->sreq_->request()->server_header()->SetChunkedTransfer(false);
    preq->sreq_->set_ready_callback(NewCallback(this,
        &Proxy::StreamingCallback, preq), 32000);
    preq->sreq_->BeginStreamingData(
        preq->sreq_->request()->server_header()->status_code(),
        NewCallback(this, &Proxy::ClearRequest, preq),
        try_chunked);
    LOG_PROXY << "PROXY:  Remote server header: \n"
              << preq->sreq_->request()->server_header()->ToString();
  }
  const int32 size_to_copy = min(size_to_send,
                                 preq->creq_->request()->server_data()->Size());
  if ( size_to_copy > 0 ) {
    preq->sreq_->request()->server_data()->AppendStreamNonDestructive(
      preq->creq_->request()->server_data(), size_to_copy);
    preq->creq_->request()->server_data()->Skip(size_to_copy);
  }
  if ( !preq->creq_->request()->server_data()->IsEmpty() ) {
    preq->proto_->PauseReading();
    preq->paused_ = true;
    preq->sreq_->set_ready_callback(NewCallback(this,
        &Proxy::StreamingCallback, preq), 32000);
    preq->sreq_->ContinueStreamingData();
  } else {
    if ( preq->paused_ ) {
      preq->proto_->ResumeReading();
      preq->paused_ = false;
    }
    preq->sreq_->set_ready_callback(NewCallback(this,
        &Proxy::StreamingCallback, preq), 32000);
    preq->sreq_->ContinueStreamingData();
  }
  if ( preq->creq_->is_finalized() &&
       preq->creq_->request()->server_data()->IsEmpty() ) {
    FinalizeRequest(preq);
    return;
  }
}

void Proxy::FinalizeRequest(ProxyReqStruct* preq) {
  if ( !preq->http_header_sent_ ) {
    preq->http_header_sent_ = true;
    const http::Header* cli_head = preq->sreq_->request()->server_header();
    if ( cli_head->status_code() == http::UNKNOWN ) {
      preq->sreq_->ReplyWithStatus(BAD_GATEWAY);
    } else {
      preq->sreq_->request()->server_header()->CopyHeaders(*cli_head, true);
      preq->sreq_->ReplyWithStatus(cli_head->status_code());
    }
  } else {
    preq->sreq_->EndStreamingData();
  }
}

void Proxy::ClearRequest(ProxyReqStruct* preq) {
  selector_->DeleteInSelectLoop(preq->proto_);
  selector_->DeleteInSelectLoop(preq->creq_);
  delete preq->callback_;
  preq->sreq_->clear_closed_callback();
  delete preq;
  pending_requests_.erase(preq);
}
}
