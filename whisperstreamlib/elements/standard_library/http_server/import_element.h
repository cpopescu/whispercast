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
#ifndef  __MEDIA_IMPORT_ELEMENT_H__
#define  __MEDIA_IMPORT_ELEMENT_H__

#include <string>
#include <map>
#include <whisperlib/net/base/address.h>
#include <whisperlib/net/rpc/lib/server/rpc_http_server.h>
#include <whisperlib/common/io/checkpoint/state_keeper.h>
#include <whisperstreamlib/base/element.h>
#include <whisperstreamlib/base/tag_splitter.h>
#include <whisperstreamlib/elements/standard_library/auto/standard_library_invokers.h>

namespace streaming {

template<class Service, class Import, class ImportDataSpec>
class ImportElement
    : public Element,
      public Service {
 protected:
  typedef map<string, Import*> ImportMap;
  typedef map<streaming::Request*, Import*> RequestMap;
 public:
  ImportElement(const string& class_name,
                const string& name,
                const string& id,
                streaming::ElementMapper* mapper,
                net::Selector* selector,
                const string& media_dir,
                const string& rpc_path,
                rpc::HttpServer* rpc_server,
                io::StateKeepUser* state_keeper,
                streaming::SplitterCreator* splitter_creator,
                const string& authorizer_name);

  virtual ~ImportElement();

  // streaming::Element interface methods
  virtual bool Initialize();
  virtual bool AddRequest(const char* media_name, streaming::Request* req,
                          streaming::ProcessingCallback* callback);
  virtual void RemoveRequest(streaming::Request* req);
  virtual bool HasMedia(const char* media, Capabilities *out);
  virtual void ListMedia(const char* media_dir,
                         streaming::ElementDescriptions* medias);
  virtual bool DescribeMedia(const string& media,
                             MediaInfoCallback* callback);
  virtual void Close(Closure* call_on_close);

  // RPC interface
  virtual void AddImport(
      rpc::CallContext<MediaOperationErrorData>* call,
      const ImportDataSpec& spec);
  virtual void DeleteImport(
      rpc::CallContext<MediaOperationErrorData>* call,
      const string& name);
  virtual void GetImports(
      rpc::CallContext< vector<string> >* call);
  virtual void GetImportDetail(
      rpc::CallContext<ImportDetail>* call,
      const string& name);

  // utility - save or not the added import ..
  void AddImport(MediaOperationErrorData* ret,
                 const ImportDataSpec& spec,
                 bool save_state);

 protected:
  virtual Import* CreateNewImport(const char* import_name,
                                  const char* name,
                                  Tag::Type tag_type,
                                  const char* save_dir,
                                  bool append_only,
                                  bool disable_preemption,
                                  int32 prefill_buffer_ms,
                                  int32 advance_media_ms,
                                  int32 buildup_interval_sec,
                                  int32 buildup_delay_sec) = 0;
 private:
  // Initializes a new server element by specifying the element name
  // and the url where to listen for media upload.
  // As long as there is no one uploading, the element has no data.
  bool AddImport(const char* import_name,
                 Tag::Type tag_type,
                 const char* save_dir,
                 bool append_only,
                 bool disable_preemption,
                 int32 prefill_buffer_ms,
                 int32 advance_media_ms,
                 int32 buildup_interval_sec,
                 int32 buildup_delay_sec);


  // Ads (first time sets :) the user/pass for pushing data through the
  // named element
  bool AddImportRemoteUser(const char* import_name,
                           const string& user, const string& passwd);
  // Deletes an exported element
  bool DeleteImport(const char* import_name);

  void CloseAllImports();

 protected:
  net::Selector* const selector_;
  const string media_dir_;
  const string rpc_path_;
  rpc::HttpServer* const rpc_server_;
  bool rpc_registered_;
  io::StateKeepUser* state_keeper_;
  streaming::SplitterCreator* splitter_creator_;
  const string authorizer_name_;

  ImportMap imports_;
  RequestMap requests_;

  Closure* close_completed_;
  DISALLOW_EVIL_CONSTRUCTORS(ImportElement);
};

}

#endif  // __MEDIA_EXPORT_ELEMENT_H__
