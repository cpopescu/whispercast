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
// Library class for standard class library
//
#ifndef __STREAMING_ELEMENTS_STANDARD_LIBRARY_STANDARD_LIBRARY_H__
#define __STREAMING_ELEMENTS_STANDARD_LIBRARY_STANDARD_LIBRARY_H__

#include <string>
#include <whisperstreamlib/elements/element_library.h>
#include <whisperstreamlib/elements/standard_library/auto/standard_library_types.h>

namespace streaming {

class ServiceInvokerStandardLibraryServiceImpl;

class StandardLibrary : public ElementLibrary {
 public:
  StandardLibrary();
  virtual ~StandardLibrary();

  // Returns in element_types the ids for the type of elements that are
  // exported through this library
  virtual void GetExportedElementTypes(vector<string>* ellement_types);

  // Returns in policy_types the ids for the type of policies that are
  // exported through this library
  virtual void GetExportedPolicyTypes(vector<string>* policy_types);

  // Returns in authorizer_types the ids for the type of authorizers that are
  // exported through this library
  virtual void GetExportedAuthorizerTypes(vector<string>* authorizer_types);

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
  virtual int64 GetAuthorizerNeeds(const string& policy_type);

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
      string* error_description);

 private:
  // Helpers for element creating Elements

  streaming::Element*
  CreateAioFileElement(const string& name,
                       const AioFileElementSpec& spec,
                       const streaming::Request* req,
                       const CreationObjectParams& params,
                       vector<string>* needed_policies,
                       string* error);
  streaming::Element*
  CreateDroppingElement(const string& name,
                        const DroppingElementSpec& spec,
                        const streaming::Request* req,
                        const CreationObjectParams& params,
                        vector<string>* needed_policies,
                        string* error);
  streaming::Element*
  CreateDebuggerElement(const string& name,
                        const DebuggerElementSpec& spec,
                        const streaming::Request* req,
                        const CreationObjectParams& params,
                        vector<string>* needed_policies,
                        string* error);
  streaming::Element*
  CreateHttpClientElement(const string& name,
                          const HttpClientElementSpec& spec,
                          const streaming::Request* req,
                          const CreationObjectParams& params,
                          vector<string>* needed_policies,
                          string* error);
  streaming::Element*
  CreateHttpPosterElement(const string& name,
                          const HttpPosterElementSpec& spec,
                          const streaming::Request* req,
                          const CreationObjectParams& params,
                          vector<string>* needed_policies,
                          string* error);
  streaming::Element*
  CreateRemoteResolverElement(const string& name,
                              const RemoteResolverElementSpec& spec,
                              const streaming::Request* req,
                              const CreationObjectParams& params,
                              vector<string>* needed_policies,
                              string* error);
  streaming::Element*
  CreateHttpServerElement(const string& name,
                          const HttpServerElementSpec & spec,
                          const streaming::Request* req,
                          const CreationObjectParams& params,
                          vector<string>* needed_policies,
                          string* error);
  streaming::Element*
  CreateRtmpPublishingElement(const string& name,
                              const RtmpPublishingElementSpec & spec,
                              const streaming::Request* req,
                              const CreationObjectParams& params,
                              vector<string>* needed_policies,
                              string* error);
  streaming::Element*
  CreateKeyFrameExtractorElement(const string& name,
                                 const KeyFrameExtractorElementSpec& spec,
                                 const streaming::Request* req,
                                 const CreationObjectParams& params,
                                 vector<string>* needed_policies,
                                 string* error);
  streaming::Element*
  CreateStreamRenamerElement(const string& name,
                             const StreamRenamerElementSpec& spec,
                             const streaming::Request* req,
                             const CreationObjectParams& params,
                             vector<string>* needed_policies,
                             string* error);
  streaming::Element*
  CreateLookupElement(const string& name,
                      const LookupElementSpec& spec,
                      const streaming::Request* req,
                      const CreationObjectParams& params,
                      vector<string>* needed_policies,
                      string* error);
  streaming::Element*
  CreateNormalizingElement(const string& name,
                           const NormalizingElementSpec& spec,
                           const streaming::Request* req,
                           const CreationObjectParams& params,
                           vector<string>* needed_policies,
                           string* error);
  streaming::Element*
  CreateLoadBalancingElement(const string& name,
                             const LoadBalancingElementSpec& spec,
                             const streaming::Request* req,
                             const CreationObjectParams& params,
                             vector<string>* needed_policies,
                             string* error);
  streaming::Element*
  CreateSavingElement(const string& name,
                      const SavingElementSpec& spec,
                      const streaming::Request* req,
                      const CreationObjectParams& params,
                      vector<string>* needed_policies,
                      string* error);
  streaming::Element*
  CreateSplittingElement(const string& name,
                         const SplittingElementSpec& spec,
                         const streaming::Request* req,
                         const CreationObjectParams& params,
                         vector<string>* needed_policies,
                         string* error);
  streaming::Element*
  CreateSwitchingElement(const string& name,
                         const SwitchingElementSpec& spec,
                         const streaming::Request* req,
                         const CreationObjectParams& params,
                         vector<string>* needed_policies,
                         string* error);
  streaming::Element*
  CreateTimeSavingElement(const string& name,
                          const TimeSavingElementSpec& spec,
                          const streaming::Request* req,
                          const CreationObjectParams& params,
                          vector<string>* needed_policies,
                          string* error);
  streaming::Element*
  CreateF4vToFlvConverterElement(const string& name,
                                 const F4vToFlvConverterElementSpec& spec,
                                 const streaming::Request* req,
                                 const CreationObjectParams& params,
                                 vector<string>* needed_policies,
                                 string* error);
  streaming::Element*
  CreateRedirectingElement(const string& name,
                           const RedirectingElementSpec& spec,
                           const streaming::Request* req,
                           const CreationObjectParams& params,
                           vector<string>* needed_policies,
                           string* error);

  // Helpers for element creating policies

  streaming::Policy*
  CreateRandomPolicy(const string& name,
                     const RandomPolicySpec& spec,
                     streaming::PolicyDrivenElement* element,
                     const streaming::Request* req,
                     const CreationObjectParams& params,
                     string* error);
  streaming::Policy*
  CreatePlaylistPolicy(const string& name,
                       const PlaylistPolicySpec& spec,
                       streaming::PolicyDrivenElement* element,
                       const streaming::Request* req,
                       const CreationObjectParams& params,
                       string* error);
  streaming::Policy*
  CreateFailoverPolicy(const string& name,
                       const FailoverPolicySpec& spec,
                       streaming::PolicyDrivenElement* element,
                       const streaming::Request* req,
                       const CreationObjectParams& params,
                       string* error);
  streaming::Policy*
  CreateOnCommandPolicy(const string& name,
                        const OnCommandPolicySpec& spec,
                        streaming::PolicyDrivenElement* element,
                        const streaming::Request* req,
                        const CreationObjectParams& params,
                        string* error);

  streaming::Policy*
  CreateTimedPlaylistPolicy(const string& name,
                            const TimedPlaylistPolicySpec& spec,
                            streaming::PolicyDrivenElement* element,
                            const streaming::Request* req,
                            const CreationObjectParams& params,
                            string* error);

  // Helpers for element creating authorizers
  streaming::Authorizer*
  CreateSimpleAuthorizer(const string& name,
                         const SimpleAuthorizerSpec& spec,
                         const CreationObjectParams& params,
                         string* error);
 private:
  ServiceInvokerStandardLibraryServiceImpl* rpc_impl_;
  DISALLOW_EVIL_CONSTRUCTORS(StandardLibrary);
};
}

#endif  // __STREAMING_ELEMENTS_STANDARD_LIBRARY_STANDARD_LIBRARY_H__
