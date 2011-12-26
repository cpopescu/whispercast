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
// Authors: Catalin Popescu

// This defines a simple interface for mapping from a stream id to a
// element.

#ifndef __MEDIA_BASE_ELEMENT_MAPPER_H__
#define __MEDIA_BASE_ELEMENT_MAPPER_H__

#include <string>
#include <whisperlib/common/base/types.h>
#include WHISPER_HASH_MAP_HEADER
#include WHISPER_HASH_SET_HEADER
#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/request.h>
#include <whisperstreamlib/base/stream_auth.h>
#include <whisperstreamlib/base/media_info.h>
#include <whisperstreamlib/base/importer.h>
#include <whisperlib/net/base/selector.h>

namespace streaming {

class ElementMapper {
 public:
  ElementMapper(net::Selector* selector)
      : selector_(selector),
        fallback_mapper_(NULL),
        master_mapper_(NULL) {
  }
  virtual ~ElementMapper() {
  }
  // If this call succeeds we own the "req" and the "callback".
  // If it fails, the "req" and "callback" are still yours.
  virtual bool AddRequest(const char* media_name,
                          streaming::Request* req,
                          streaming::ProcessingCallback* callback) = 0;

  // "req" and "callback" are automatically deleted.
  virtual void RemoveRequest(streaming::Request* req,
                             ProcessingCallback* callback) = 0;

  virtual void GetMediaDetails(const string& protocol,
                               const string& path,
                               streaming::Request* req,
                               Callback1<bool>* completion_callback) = 0;
  virtual Authorizer* GetAuthorizer(const string& name) = 0;

  virtual bool HasMedia(const char* media_name, Capabilities* out) = 0;
  virtual void ListMedia(const char* media_dir,
                         streaming::ElementDescriptions* medias) = 0;

  typedef hash_set<streaming::Element*> ElementSet;
  typedef hash_map<streaming::Element*, vector<streaming::Policy*>*> PolicyMap;
  virtual bool GetElementByName(const string& name,
                                streaming::Element** element,
                                vector<streaming::Policy*>** policies) = 0;
  virtual bool IsKnownElementName(const string& name) = 0;
  virtual bool GetMediaAlias(const string& alias_name,
                             string* media_name) const = 0;
  virtual string TranslateMedia(const char* media_name) const = 0;

  // asynchronous method.
  // returns: true => MediaInfo will be delivered through the given 'callback'.
  //          false => failure. The 'callback' is not called.
  typedef Callback1<const MediaInfo*> MediaInfoCallback;
  virtual bool DescribeMedia(const string& media,
      MediaInfoCallback* callback) = 0;

  void set_fallback_mapper(ElementMapper* fallback_mapper) {
    CHECK(fallback_mapper != this);
    fallback_mapper_ = fallback_mapper;
  }
  void set_master_mapper(ElementMapper* master_mapper) {
    CHECK(master_mapper != this);
    master_mapper_ = master_mapper;
  }
  // Increment client count for a given export.
  // Useful in limiting client count / export.
  // Returns: the number of clients connected to the "export_path".
  virtual int32 AddExportClient(const string& protocol,
                                const string& export_path) = 0;
  // Decrement client count for a given export.
  virtual void RemoveExportClient(const string& protocol,
                                  const string& export_path) = 0;

  // Importer ( Publishing ). An Importer is actually an Element.
  // Called by the element, to make himself available to network.
  virtual bool AddImporter(Importer* importer) = 0;
  virtual void RemoveImporter(Importer* importer) = 0;
  // Called by the network protocol, to obtain an importer
  // and then send him tags. Must be called in media selector.
  virtual Importer* GetImporter(Importer::Type importer_type,
                                const string& path) = 0;

 protected:
  net::Selector* const selector_;
  ElementMapper* fallback_mapper_;
  ElementMapper* master_mapper_;

  DISALLOW_EVIL_CONSTRUCTORS(ElementMapper);
};
}

#endif  // __MEDIA_BASE_ELEMENT_MAPPER_H__
