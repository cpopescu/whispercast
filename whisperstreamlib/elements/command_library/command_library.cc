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

#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_decoder.h>
#include <whisperlib/common/io/buffer/io_memory_stream.h>
#include <whisperlib/common/io/ioutil.h>

#include <whisperstreamlib/base/consts.h>
#include <whisperstreamlib/elements/command_library/command_library.h>
#include <whisperstreamlib/elements/command_library/command_element.h>
#include <whisperstreamlib/elements/command_library/auto/command_library_invokers.h>

//////////////////////////////////////////////////////////////////////

// Library Stub

#ifdef __cplusplus
extern "C" {
#endif

  void* GetElementLibrary() {
    return new streaming::CommandLibrary();
  }

#ifdef __cplusplus
}
#endif

//////////////////////////////////////////////////////////////////////

namespace streaming {

CommandLibrary::CommandLibrary()
    : ElementLibrary("command_library"),
      rpc_impl_(NULL) {
}

CommandLibrary::~CommandLibrary() {
  CHECK(rpc_impl_ == NULL)
      << " Deleting the library before unregistering the rpc";
}

void CommandLibrary::GetExportedElementTypes(vector<string>* element_types) {
  element_types->push_back(CommandElement::kElementClassName);
}

void CommandLibrary::GetExportedPolicyTypes(vector<string>* policy_types) {
}

int64 CommandLibrary::GetElementNeeds(const string& element_type) {
  if ( element_type == CommandElement::kElementClassName ) {
    return NEED_SELECTOR;
  }
  return 0;
}

int64 CommandLibrary::GetPolicyNeeds(const string& policy_type) {
  return 0;
}

streaming::Element* CommandLibrary::CreateElement(
    const string& element_type,
    const string& name,
    const string& element_params,
    const streaming::Request* req,
    const CreationObjectParams& params,
    bool is_temporary_template,
    // Output:
    vector<string>* needed_policies,
    string* error_description) {
  streaming::Element* ret = NULL;
  if ( element_type == CommandElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(Command);
    return ret;
  }
  LOG_ERROR << "Dunno to create element of type: '" << element_type << "'";
  return NULL;
}

streaming::Policy* CommandLibrary::CreatePolicy(
      const string& policy_type,
      const string& name,
      const string& policy_params,
      streaming::PolicyDrivenElement* element,
      const streaming::Request* req,
      const CreationObjectParams& params,
      // Output:
      string* error_description) {
  return NULL;
}

streaming::Element* CommandLibrary::CreateCommandElement(
    const string& element_name,
    const CommandElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    bool is_temporary_template,
    string* error) {
  CHECK(mapper_ != NULL);
  MediaFormat media_format;
  if ( !MediaFormatFromSmallType(spec.media_type_.get(), &media_format) ) {
    *error = "Invalid media type specified for CommandElement";
    return NULL;
  }
  CommandElement* element = new CommandElement(element_name.c_str(),
                                               mapper_,
                                               params.selector_);
  for ( int i = 0; spec.commands_.get().size() > i; ++i )  {
    const CommandSpec& crt_spec = spec.commands_.get()[i];
    if ( !element->AddElement(crt_spec.name_.get().c_str(),
                              media_format,
                              crt_spec.command_.get().c_str(),
                              crt_spec.should_reopen_.get()) ) {
      // TODO(cpopescu): should we report an error on this one ?
      LOG_ERROR << "Cannot AddElement " << crt_spec.ToString()
                << "  for CommandLibrary " << element_name;
    }
  }
  return element;
}

//////////////////////////////////////////////////////////////////////

class ServiceInvokerCommandLibraryServiceImpl
    : public ServiceInvokerCommandLibraryService {
 public:
  ServiceInvokerCommandLibraryServiceImpl(
      streaming::ElementMapper* mapper,
      streaming::ElementLibrary::ElementSpecCreationCallback*
      create_element_spec_callback,
      streaming::ElementLibrary::PolicySpecCreationCallback*
      create_policy_spec_callback)
      : ServiceInvokerCommandLibraryService(
          ServiceInvokerCommandLibraryService::GetClassName()),
        mapper_(mapper),
        create_element_spec_callback_(create_element_spec_callback),
        create_policy_spec_callback_(create_policy_spec_callback) {
  }

 private:
  virtual void AddCommandElementSpec(
      rpc::CallContext<MediaOpResult>* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const CommandElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(Command);
  }
 private:
  streaming::ElementMapper* mapper_;
  streaming::ElementLibrary::ElementSpecCreationCallback*
  create_element_spec_callback_;
  streaming::ElementLibrary::PolicySpecCreationCallback*
  create_policy_spec_callback_;

  DISALLOW_EVIL_CONSTRUCTORS(ServiceInvokerCommandLibraryServiceImpl);
};

//////////////////////////////////////////////////////////////////////

bool CommandLibrary::RegisterLibraryRpc(rpc::HttpServer* rpc_server,
                                        string* rpc_http_path) {
  CHECK(rpc_impl_ == NULL);
  rpc_impl_ = new ServiceInvokerCommandLibraryServiceImpl(
      mapper_, create_element_spec_callback_, create_policy_spec_callback_);
  if ( !rpc_server->RegisterService(name() + "/", rpc_impl_) ) {
    delete rpc_impl_;
    return false;
  }
  *rpc_http_path = strutil::NormalizePath(
      rpc_server->path() + "/" + name() + "/" + rpc_impl_->GetName());
  return true;
}

bool CommandLibrary::UnregisterLibraryRpc(rpc::HttpServer* rpc_server) {
  CHECK(rpc_impl_ != NULL);
  const bool success = rpc_server->UnregisterService(name() + "/", rpc_impl_);
  delete rpc_impl_;
  rpc_impl_ = NULL;
  return success;
}
}
