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
// Library class for CommandElement - a potentialy dangerous element.
//
#ifndef __STREAMING_ELEMENTS_COMMAND_LIBRARY_COMMAND_LIBRARY_H__
#define __STREAMING_ELEMENTS_COMMAND_LIBRARY_COMMAND_LIBRARY_H__

#include <string>
#include <whisperstreamlib/elements/element_library.h>
#include <whisperstreamlib/elements/command_library/auto/command_library_types.h>

namespace streaming {

class ServiceInvokerCommandLibraryServiceImpl;

class CommandLibrary : public ElementLibrary {
 public:
  CommandLibrary();
  virtual ~CommandLibrary();

  // Returns in element_types the ids for the type of elements that are
  // exported through this library
  virtual void GetExportedElementTypes(vector<string>* ellement_types);

  // Returns in policy_types the ids for the type of policies that are
  // exported through this library
  virtual void GetExportedPolicyTypes(vector<string>* policy_types);

  // Returns in authorizer_types the ids for the type of authorizers that are
  // exported through this library
  virtual void GetExportedAuthorizerTypes(vector<string>* authorizer_types) {
    return;
  }

  // Registers any global RPC-s necessary for the library
  virtual bool RegisterLibraryRpc(rpc::HttpServer* rpc_server,
                                  string* rpc_http_path);
  // The opposite of RegisterLibraryRpc
  virtual bool UnregisterLibraryRpc(rpc::HttpServer* rpc_server);

  // Returns a combination of Needs (OR-ed), for what is needed in order
  // to create an element of given type
  virtual int64 GetElementNeeds(const string& element_type);

  // Returns a combination of Needs (OR-ed), for what is needed in order
  // to a policy  of given type
  virtual int64 GetPolicyNeeds(const string& element_type);

  // Returns a combination of Needs (OR-ed), for what is needed in order
  // to create an authorizer of a given type
  virtual int64 GetAuthorizerNeeds(const string& policy_type) {
    return 0;
  }


  // Main creation function for an element.
  virtual streaming::Element* CreateElement(
      const string& element_type,
      const string& name,
      const string& element_params,
      const streaming::Request* req,
      const CreationObjectParams& params,
      // Output:
      vector<string>* needed_policies,
      string* error_description);
  // Main creation function for a policy.
  virtual streaming::Policy* CreatePolicy(
      const string& policy_type,
      const string& name,
      const string& policy_params,
      streaming::PolicyDrivenElement* element,
      const streaming::Request* req,
      const CreationObjectParams& params,
      // Output:
      string* error_description) ;
  // Main creation function for an authorizer
  virtual streaming::Authorizer* CreateAuthorizer(
      const string& authorizer_type,
      const string& name,
      const string& authorizer_params,
      const CreationObjectParams& params,
      // Output:
      string* error_description) {
    return NULL;
  }

 private:
  // Helpers for element creating Elements

  streaming::Element*
  CreateCommandElement(const string& name,
                       const CommandElementSpec& spec,
                       const streaming::Request* req,
                       const CreationObjectParams& params,
                       vector<string>* needed_policies,
                       string* error);

 private:
  ServiceInvokerCommandLibraryServiceImpl* rpc_impl_;
  DISALLOW_EVIL_CONSTRUCTORS(CommandLibrary);
};
}

#endif  // __STREAMING_ELEMENTS_COMMAND_LIBRARY_COMMAND_LIBRARY_H__
