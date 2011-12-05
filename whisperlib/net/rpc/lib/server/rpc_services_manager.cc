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

#include "net/rpc/lib/server/rpc_services_manager.h"
#include "net/rpc/lib/types/rpc_all_types.h"

namespace rpc {

rpc::ServicesManager::ServicesManager()
    : services_() {
}

rpc::ServicesManager::~ServicesManager() {
  CHECK(services_.empty());
}

string rpc::ServicesManager::StrListServices(const string& glue) const {
  // compute result string length
  uint32 result_len = services_.empty() ? 0
                      : ((services_.size() - 1) * glue.length());
  for ( ServicesMap::const_iterator it = services_.begin();
        it != services_.end(); ++it ) {
    rpc::ServiceInvoker& invoker = *it->second;
    result_len += invoker.GetName().length();
  }

  // allocate result
  string result;
  result.reserve(result_len + 1);

  // build result
  for ( ServicesMap::const_iterator it = services_.begin();
        it != services_.end(); ) {
    rpc::ServiceInvoker& invoker = *it->second;
    result.append(invoker.GetName());
    ++it;
    if ( it != services_.end() ) {
      result.append(glue);
    }
  }

  return result;
}
rpc::ServiceInvoker* rpc::ServicesManager::FindService(
    const string& serviceName) {
  ServicesMap::iterator it = services_.find(serviceName);
  if ( it == services_.end() ) {
    return NULL;
  }
  return it->second;
}

bool rpc::ServicesManager::RegisterService(rpc::ServiceInvoker& service) {
  // try to insert the pair [serviceName, &service] the services_ map
  //
  pair<ServicesMap::iterator, bool>
      p  = services_.insert(make_pair(service.GetName(), &service));
  if ( p.second == false ) {
    // The Key already exists in the map. The Value has NOT been replaced.

    // Service name already registered.
    //
    const rpc::ServiceInvoker* existingService = p.first->second;
    CHECK(existingService);
    LOG_ERROR << "ServiceName: " << service.GetName()
              << " already registered with service: "
              << existingService->ToString();
    return false;
  }

  // the pair [serviceName, &service] was successfully put into map
  //
  // debug purpose: mark the service as registered so it cannot be
  //                silently deleted
  service.SetRegistered(true);

  LOG_INFO << "Registered: " << service.ToString();
  return true;
}

void rpc::ServicesManager::UnregisterService(const string& serviceName) {
  ServicesMap::iterator it = services_.find(serviceName);
  if ( it == services_.end() ) {
    LOG_ERROR << "Service " << serviceName
              << " not registered. Cannot unregister.";
    return;
  }

  // check service sanity
  //
  rpc::ServiceInvoker* service = it->second;
  CHECK_NOT_NULL(service);
  CHECK_EQ(serviceName, service->GetName());

  // remove the service
  //
  services_.erase(it);

  // debug purpose: mark the service as unregistered.
  //
  service->SetRegistered(false);
  LOG_INFO << "Unregistered: " << service->ToString();
}

void rpc::ServicesManager::UnregisterService(
    const rpc::ServiceInvoker& service) {
  UnregisterService(service.GetName());
}

bool rpc::ServicesManager::Call(rpc::Query* q) {
  LOG_INFO << "Call:"
           << " service=" << q->service()
           << " method=" << q->method();
  rpc::ServiceInvoker* service = FindService(q->service());
  if ( !service ) {
    LOG_WARNING << "Cannot find service: " << q->service();
    q->Complete(RPC_SERVICE_UNAVAIL);
    return true;
  }
  return service->Call(q);
}

string rpc::ServicesManager::ToString() const {
  ostringstream oss;
  oss << "rpc::ServicesManager: #" << services_.size()
      << " services: {" << StrListServices(", ") << "}";
  return oss.str();
}
}
