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
// Here is what an element library should export:
//
//  - An 'extern c' function that returns a pointer to a newly created
//    media::ElementLibrary implementation.
//
// Your element library entry point should look like this:
//
// #ifdef __cplusplus
// extern "C" {
// #endif
//
// void* GetElementLibrary() {
//   return new MyElementLibrary(...);
// }
//
// #ifdef __cplusplus
// }
// #endif
//
#ifndef __STREAMING_ELEMENTS_LIBRARY_ELEMENT_LIBRARY_H__
#define __STREAMING_ELEMENTS_LIBRARY_ELEMENT_LIBRARY_H__

#include <vector>
#include <string>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/io/file/buffer_manager.h>
#include <whisperlib/common/io/file/aio_file.h>
#include <whisperlib/common/io/checkpoint/state_keeper.h>
#include <whisperlib/net/base/selector.h>
#include <whisperlib/net/http/http_server_protocol.h>
#include <whisperlib/net/rpc/lib/server/rpc_http_server.h>
#include <whisperstreamlib/base/element.h>
#include <whisperstreamlib/base/stream_auth.h>
#include <whisperstreamlib/base/tag_splitter.h>

namespace streaming {

class ElementLibrary {
 public:
  ElementLibrary(const string& name)
      : mapper_(NULL),
        create_element_spec_callback_(NULL),
        create_policy_spec_callback_(NULL),
        create_authorizer_spec_callback_(NULL),
        name_(name) {
  }

  virtual ~ElementLibrary() {
  }

  // The name of this library - so simple :) Unique, and valid identifier:
  // only alphanumeric and _
  const string& name() {
    return name_;
  }


  // Returns in element_types the ids for the type of elements that are
  // exported through this library
  virtual void GetExportedElementTypes(vector<string>* ellement_types) = 0;

  // Returns in policy_types the ids for the type of policies that are
  // exported through this library
  virtual void GetExportedPolicyTypes(vector<string>* policy_types) = 0;

  // Returns in authorizer_types the ids for the type of authorizers that are
  // exported through this library
  virtual void GetExportedAuthorizerTypes(vector<string>* authorizer_types) = 0;

  // Registers any global RPC-s necessary for the library
  virtual bool RegisterLibraryRpc(rpc::HttpServer* rpc_server,
                                  string* rpc_http_path) = 0;
  // The opposite of RegisterLibraryRpc
  virtual bool UnregisterLibraryRpc(rpc::HttpServer* rpc_server) = 0;

  // These are library needs in order to create an element:
  enum Needs {
    NEED_SELECTOR         = 0x0001,
    NEED_HTTP_SERVER      = 0x0002,
    NEED_RPC_SERVER       = 0x0004,
    NEED_STATE_KEEPER     = 0x0008,
    NEED_AIO_FILES        = 0x0010,
    NEED_SPLITTER_CREATOR = 0x0020,
    NEED_HOST2IP_MAP      = 0x0040,
    NEED_MEDIA_DIR        = 0x0080,
  };
  // Returns a combination of Needs (OR-ed), for what is needed in order
  // to create an element of given type
  virtual int64 GetElementNeeds(const string& element_type) = 0;

  // Returns a combination of Needs (OR-ed), for what is needed in order
  // to create a policy  of given type
  virtual int64 GetPolicyNeeds(const string& policy_type) = 0;

  // Returns a combination of Needs (OR-ed), for what is needed in order
  // to create an authorizer of a given type
  virtual int64 GetAuthorizerNeeds(const string& policy_type) = 0;


  // Parameters for creating an element. You can use them but you cannot delete
  // them except:
  //   state_keeper_
  //   splitter_creator_
  //
  struct CreationObjectParams {
    net::Selector* selector_;            // NULL if NEED_SELECTOR is off
    http::Server* http_server_;          // NULL if NEED_HTTP_SERVER is off
    rpc::HttpServer* rpc_server_;        // NULL if NEED_RPC_SERVER is off
    map<string, io::AioManager*>* aio_managers_;
                                         // NULL if NEED_AIO_FILES is off
    io::BufferManager* buffer_manager_;
                                         // NULL if NEED_AIO_FILES is off
    const map<string, string>* host_aliases_;
                                         // NULL if NEED_HOST2IP_MAP is off
                                         // or no aliases available
    string media_dir_;                   // Root of all media that can be
                                         // served
    // THINGS YOU OWN -- responsible for deleting them..
    io::StateKeepUser* state_keeper_;    // NULL if NEED_STATE_KEEPER is off
    io::StateKeepUser* local_state_keeper_;
                                         // NULL if NEED_STATE_KEEPER is off
                                         // or for global playlists
    streaming::SplitterCreator* splitter_creator_;
                                         // NULL if NEED_SPLITTER_CREATOR is off
    CreationObjectParams()
        : selector_(NULL),
          http_server_(NULL),
          rpc_server_(NULL),
          aio_managers_(NULL),
          buffer_manager_(NULL),
          host_aliases_(NULL),
          media_dir_(),
          state_keeper_(NULL),
          local_state_keeper_(NULL),
          splitter_creator_(NULL) {
    }
  };

  // Main creation function for an element.
  //
  // Return: the newly created element, NULL if some error was encountered
  //         (in that case you should set also a description for the error
  //         in error_description);
  //
  // element_type  -- create an element of this type
  // name          -- give it this name
  // element_params -- parameters (possible encoded things) used in element
  //                  creation.
  // needed_policies  -- if you created a PolicyDriven element, you must return
  //                     the policy types that you need
  // req -- if this element is temporarely (created when on a user request
  //        is processed), this is the request that caused it; else is NULL
  // params -- general parameters (see above).
  // needed_policies -- If the element you create is a PolicyDrivenElement,
  //                    you can add here the policies that need to be created
  //                    immediatly after this. Of course, if you add something
  //                    here you *must* return a PolicyDrivenElement..
  // error_description - In case of errors, you should set a description here
  //
  virtual streaming::Element* CreateElement(
      const string& element_type,
      const string& name,
      const string& element_params,
      const streaming::Request* req,
      const CreationObjectParams& params,
      // Output:
      vector<string>* needed_policies,
      string* error_description) = 0;
  // Main creation function for a policy.
  //
  // Return: the newly created policy, NULL if some error was encountered
  //         (in that case you should set also a description for the error
  //         in error_description).
  //
  virtual streaming::Policy* CreatePolicy(
      const string& policy_type,
      const string& name,
      const string& policy_params,
      streaming::PolicyDrivenElement* element,
      const streaming::Request* req,
      const CreationObjectParams& params,
      // Output:
      string* error_description) = 0;

  // Main creation function for an authorizer.
  //
  // Return: the newly created authorizer, NULL if some error was encountered
  //         (in that case you should set also a description for the error
  //         in error_description).
  //
  virtual streaming::Authorizer* CreateAuthorizer(
      const string& authorizer_type,
      const string& name,
      const string& authorizer_params,
      const CreationObjectParams& params,
      // Output:
      string* error_description) = 0;

  //////////////////////////////////////////////////////////////////////

  struct ElementSpecCreateParams {
    string type_;      // the type of this element (supported by this lib)
    string name_;      // name identifier..
    bool is_global_;   // is the element global (used for all requests)
                       // vs. being created and used temporarely, per request
    bool disable_rpc_;        // if on, we disable rpc for the element
    string element_params_;   // each element library knows what this means
                              // (it can be a JSON encoded parameter list

    string error_;
  };
  struct PolicySpecCreateParams {
    string type_;      // the type of this policy (supported by this lib)
    string name_;      // name identifier for the spec
    string policy_params_;    // each element library knows what this means
                              // (it can be a JSON encoded parameter list

    string error_;
  };
  struct AuthorizerSpecCreateParams {
    string type_;      // the type of this authorizer (supported by this lib)
    string name_;      // name identifier for the spec
    string authorizer_params_;    // each element library knows what this means
                                  // (it can be a JSON encoded parameter list

    string error_;
  };

  typedef ResultCallback1<bool,
                          ElementSpecCreateParams*>
  ElementSpecCreationCallback;
  typedef ResultCallback1<bool,
                          PolicySpecCreateParams*>
  PolicySpecCreationCallback;
  typedef ResultCallback1<bool,
                          AuthorizerSpecCreateParams*>
  AuthorizerSpecCreationCallback;

  // Sets the parameters bellow (NOTE: these are not your pointers to delete)
  void SetParameters(
      streaming::ElementMapper* mapper,
      ElementSpecCreationCallback* create_element_spec_callback,
      PolicySpecCreationCallback* create_policy_spec_callback,
      AuthorizerSpecCreationCallback* create_authorizer_spec_callback) {
    mapper_ = mapper;
    create_element_spec_callback_ = create_element_spec_callback;
    create_policy_spec_callback_ = create_policy_spec_callback;
    create_authorizer_spec_callback_ = create_authorizer_spec_callback;
  }

 protected:
  // keeps track of all elements and policies in the game
  streaming::ElementMapper* mapper_;
  // You can use these to add element and policy specs to the configuration.
  // The ideea is that only the library knows about the parameters for
  // the element / policy creation so it can register, customary, a
  // standard names rpc service for adding/removing policies from the
  // configuration.
  ElementSpecCreationCallback* create_element_spec_callback_;
  PolicySpecCreationCallback* create_policy_spec_callback_;
  AuthorizerSpecCreationCallback* create_authorizer_spec_callback_;
 private:
  const string name_;
  DISALLOW_EVIL_CONSTRUCTORS(ElementLibrary);
};
}


//////////////////////////////////////////////////////////////////////

// Some macros that you may find useful
//   (see standard_library/standard_library.cc for an example of use);
//
// You need:
// #include "net/rpc/lib/codec/json/rpc_json_encoder.h"
// #include "net/rpc/lib/codec/json/rpc_json_decoder.h"
// #include "common/io/buffer/io_memory_stream.h"
// and declare
// streaming::Element* ret = NULL;

#define CREATE_ELEMENT_HELPER(type)                                     \
  do {                                                                  \
    type##ElementSpec spec;                                             \
    if ( !rpc::JsonDecoder::DecodeObject(element_params, &spec) ) {     \
      *error_description = "Error decoding "#type"ElementSpec params";  \
      LOG_ERROR << "Error decoding "#type"ElementSpec params from: "    \
                << element_params;                                      \
      return NULL;                                                      \
    }                                                                   \
    ret = Create##type##Element(name, spec, req,                        \
                                params, needed_policies,                \
                                error_description);                     \
  } while ( false )

#define CREATE_POLICY_HELPER(type)                                      \
  do {                                                                  \
    type##PolicySpec spec;                                              \
    if ( !rpc::JsonDecoder::DecodeObject(policy_params, &spec) ) {      \
      *error_description = "Error decoding "#type"PolicySpec params";   \
      LOG_ERROR << "Error decoding "#type"PolicySpec params from: "     \
                << policy_params;                                       \
      return NULL;                                                      \
    }                                                                   \
    ret = Create##type##Policy(name, spec, element, req, params,        \
                               error_description);                      \
  } while ( false )

#define CREATE_AUTHORIZER_HELPER(type)                                  \
  do {                                                                  \
    type##AuthorizerSpec spec;                                          \
    if ( !rpc::JsonDecoder::DecodeObject(authorizer_params, &spec) ) {  \
      *error_description = "Error decoding "#type"AuthorizerSpec params"; \
      LOG_ERROR << "Error decoding "#type"AuthorizerSpec params from: " \
                << authorizer_params;                                   \
      return NULL;                                                      \
    }                                                                   \
    ret = Create##type##Authorizer(name, spec, params,                  \
                                   error_description);                  \
  } while ( false )

#define STANDARD_RPC_ELEMENT_ADD(type)                                  \
  do {                                                                  \
    MediaOperationErrorData ret;                                        \
    streaming::ElementLibrary::ElementSpecCreateParams params;          \
    params.name_ = name;                                                \
    params.type_ = type##Element::kElementClassName;                    \
    params.is_global_ = is_global;                                      \
    params.disable_rpc_ = disable_rpc;                                  \
    params.element_params_ = rpc::JsonEncoder::EncodeObject(spec);      \
    if ( !create_element_spec_callback_->Run(&params) ) {               \
      ret.error_.ref() = 1;                                             \
      ret.description_.ref() = params.error_;                           \
    } else {                                                            \
      ret.error_.ref() = 0;                                             \
    }                                                                   \
    call->Complete(ret);                                                \
  } while ( false );

#define STANDARD_RPC_POLICY_ADD(type)                                   \
  do {                                                                  \
    MediaOperationErrorData ret;                                        \
    streaming::ElementLibrary::PolicySpecCreateParams params;           \
    params.name_ = name;                                                \
    params.type_ = type##Policy::kPolicyClassName;                      \
    params.policy_params_ = rpc::JsonEncoder::EncodeObject(spec);       \
    if ( !create_policy_spec_callback_->Run(&params) ) {                \
      ret.error_.ref() = 1;                                             \
      ret.description_.ref() = params.error_;                           \
    } else {                                                            \
      ret.error_.ref() = 0;                                             \
    }                                                                   \
    call->Complete(ret);                                                \
  } while ( false );

#define STANDARD_RPC_AUTHORIZER_ADD(type)                               \
  do {                                                                  \
    MediaOperationErrorData ret;                                        \
    streaming::ElementLibrary::AuthorizerSpecCreateParams params;       \
    params.name_ = name;                                                \
    params.type_ = type##Authorizer::kAuthorizerClassName;              \
    params.authorizer_params_ = rpc::JsonEncoder::EncodeObject(spec);   \
    if ( !create_authorizer_spec_callback_->Run(&params) ) {            \
      ret.error_.ref() = 1;                                             \
      ret.description_.ref() = params.error_;                           \
    } else {                                                            \
      ret.error_.ref() = 0;                                             \
    }                                                                   \
    call->Complete(ret);                                                \
  } while ( false );

#endif //  __STREAMING_ELEMENTS_LIBRARY_ELEMENT_LIBRARY_H__
