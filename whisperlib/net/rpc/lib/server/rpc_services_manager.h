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

#ifndef __NET_RPC_LIB_SERVER_RPC_SERVICES_MANAGER_H__
#define __NET_RPC_LIB_SERVER_RPC_SERVICES_MANAGER_H__

#include <map>
#include <string>

#include <whisperlib/net/rpc/lib/server/rpc_service_invoker.h>
#include <whisperlib/net/rpc/lib/server/rpc_core_types.h>

// This guy keeps all the RPCServices. He receives RPC calls and directs them
// to the appropiate RPCService for execution.

namespace rpc {

class ServicesManager {
 public:
  ServicesManager();
  virtual ~ServicesManager();

  //  Returns a list of registered services. Elements are separated by "glue".
  string StrListServices(const string& glue) const;

  //  Find service by name.
  rpc::ServiceInvoker* FindService(const string& serviceName);

  //  Register a new remote service.
  //  The service name is given by service.GetName() .
  //
  //  Registering a "service" S having name N means that calling local method
  //  Call(N, method, params, ..) will actually invoke
  //  S.Call(method, params, ..) .
  // input:
  //   service: the service to register.
  // returns:
  //   Success status. It is illegal to register the same service twice.
  // NOTE: you must NOT delete a registered service!
  //       Call UnregisterService(..) before deleting the service.
  bool RegisterService(rpc::ServiceInvoker& service);

  // Unregister remote service by service name.
  // This is the reverse of the RegisterService(service) method.
  void UnregisterService(const string& serviceName);

  // Unregister remote service. You must provide the same service object here,
  // that was used on RegisterService(service).
  void UnregisterService(const rpc::ServiceInvoker& service);


  //  Executes the RPC query by finding the corresponding service,
  //  and passeing query execution to that service.
  // input:
  //  [IN/OUT] q: contains the query to be executed (method, params) and also
  //              has a completion method which forwards the result to
  //              the transport layer.
  //              After the query is executed, call q->Complete(return
  //              value, status)
  //               Return value can be any rpc::Object.
  //               Status possible values:
  //                  - RPC_SUCCESS = success.
  //                  - RPC_SERVICE_UNAVAIL = no such service.
  //                  - RPC_PROC_UNAVAIL = no such procedure.
  //                  - RPC_GARBAGE_ARGS = bad parameters.
  //                  - RPC_SYSTEM_ERR = unpredictable errors,
  //                                     like out of memory.
  // returns:
  //  true = execution succeeded. However the call may have failed.
  //         E.g. If the parameters are wrong, this Call should execute
  //              q->Complete(rpc::Void(), RPC_GARBAGE_ARGS) and return true.
  //  false = internal error. q->Complete has not been called.
  //          Check GetLastError() for more info.
  //          This should probably never happen.
  bool Call(rpc::Query* query);

  //  Describes the manager.
  string ToString() const;
 protected:
  // Map of services. ["service name" -> service invoker]
  typedef map<string, ServiceInvoker*> ServicesMap;
  ServicesMap services_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ServicesManager);
};
}
#endif   // __NET_RPC_LIB_SERVER_RPC_SERVICES_MANAGER_H__
