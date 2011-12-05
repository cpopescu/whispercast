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

#ifndef __NET_RPC_LIB_SERVER_RPC_SERVICE_INVOKER_H__
#define __NET_RPC_LIB_SERVER_RPC_SERVICE_INVOKER_H__

#include <string>
#include <whisperlib/net/rpc/lib/types/rpc_message.h>
#include <whisperlib/net/rpc/lib/server/rpc_core_types.h>

// This is an interface for all rpc services. Every RPC service must implement
// this interface in order to operate with the rpc::ServicesManager

namespace rpc {
class ServicesManager;
class ServiceInvoker {
 public:
  ServiceInvoker(const string& service_class_name,
                 const string& service_instance_name)
      : service_class_name_(service_class_name),
        service_instance_name_(service_instance_name) {
  }
  virtual ~ServiceInvoker() {
  }

  // Returns service class.
  const string& GetClassName() const {
    return service_class_name_;
  }

  // Returns service name.
  const string& GetName() const {
    return service_instance_name_;
  }

  // Call a specific method of this service:
  //   method(params) -> [status, return value]
  //   Please check: rpc::ServicesManager::Call for a more detail description of
  //                 in and out parameters as the function relate (one calls
  //                 the other)
  virtual bool Call(rpc::Query * q) = 0;

  // Helper that allows the display of form for RPC requests directly
  // from the server.
  // base_path - the path on which we export the service (e.g. /rpc)
  // url_path  - the path received from client (e.g. from
  //              /rpc/MyService/__form_GetSomeStuff -> "__form_GetSomeStuff")
  //
  // we return empty string if we do not recognize the service.
  virtual string GetTurntablePage(const string& base_path,
                                  const string& url_path) const = 0;

  //////////////////////////////////////////////////////////////////////
  //
  //                rpc::Loggable interface methods
  //
  string ToString() const {
    return string("RPCServiceInvoker ") + GetName();
  }

 protected:
  // the service class name (e.g. "Logger")
  const string service_class_name_;
  // a custom name for this instance (e.g. "log1", "log2")
  const string service_instance_name_;

  //////////////////////////////////////////////////////////////////////
  //
  // bug-trap. Mark registered services to prevent accidental deletion.
  //
  bool registered_;

  void SetRegistered(bool registered) {
    registered_ = registered;
  }

  friend class rpc::ServicesManager;

  //////////////////////////////////////////////////////////////////////

  DISALLOW_EVIL_CONSTRUCTORS(ServiceInvoker);
};
}
#endif  // __NET_RPC_LIB_SERVER_RPC_SERVICE_INVOKER_H__
