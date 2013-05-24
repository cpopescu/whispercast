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

#ifndef  __MEDIA_IMPORT_ELEMENT_H__
#define  __MEDIA_IMPORT_ELEMENT_H__

#include <string>
#include <map>
#include <whisperlib/net/base/address.h>
#include <whisperlib/net/rpc/lib/server/rpc_http_server.h>
#include <whisperlib/common/io/checkpoint/state_keeper.h>
#include <whisperstreamlib/base/element.h>

namespace streaming {

class ImportElementData {
 public:
  ImportElementData() {}
  virtual ~ImportElementData() {}

  // importer media info
  virtual const MediaInfo* media_info() const = 0;
  // Initialize the importer.
  // If it fails, the importer is deleted without calling Close()
  virtual bool Initialize() = 0;
  // Self explanatory
  virtual bool AddCallback(Request* request, ProcessingCallback* callback) = 0;
  // Self explanatory
  virtual void RemoveCallback(Request* request) = 0;
  // Completely close the importer. This call must stop everything inside the
  // importer because on return this ImportElementData* will be deleted.
  virtual void Close() = 0;
};

class ImportElement : public Element {
 protected:
  typedef map<string, ImportElementData*> ImportMap;
  typedef map<streaming::Request*, ImportElementData*> RequestMap;
 public:
  ImportElement(const string& class_name,
                const string& name,
                ElementMapper* mapper,
                net::Selector* selector,
                const string& rpc_path,
                rpc::HttpServer* rpc_server,
                io::StateKeepUser* state_keeper,
                const string& authorizer_name);
  virtual ~ImportElement();

  ////////////////////////////////////////////////////////////////////////////
  // streaming::Element interface methods
  virtual bool Initialize();
  virtual bool AddRequest(const string& media, Request* req,
                          ProcessingCallback* callback);
  virtual void RemoveRequest(Request* req);
  virtual bool HasMedia(const string& media);
  virtual void ListMedia(const string& media_dir, vector<string>* out);
  virtual bool DescribeMedia(const string& media, MediaInfoCallback* callback);
  virtual void Close(Closure* call_on_close);
  ///////////////////////////////////////////////////////////////////////////

  // Initializes a new server branch where to listen for media upload.
  // As long as there is no one uploading, the element has no data.
  bool AddImport(const string& import_name, bool save_state, string* out_error);

  // Deletes a server branch
  bool DeleteImport(const string& import_name);

  // returns the name of all the imports
  void GetAllImports(vector<string>* out) const;

  // Save/Load imports to state_keeper
  void SaveState();
  bool LoadState();

 private:
  virtual ImportElementData* CreateNewImport(const string& name) = 0;
  void CloseAllImports();

 protected:
  net::Selector* const selector_;
  const string rpc_path_;
  rpc::HttpServer* const rpc_server_;
  io::StateKeepUser* state_keeper_;
  const string authorizer_name_;

  ImportMap imports_;
  RequestMap requests_;

  Closure* close_completed_;
  DISALLOW_EVIL_CONSTRUCTORS(ImportElement);
};

}

#endif  // __MEDIA_EXPORT_ELEMENT_H__
