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
// Author: Cosmin Tudorache & Catalin Popescu

#ifndef __MEDIA_BASE_MEDIA_FILTERING_ELEMENT_H__
#define __MEDIA_BASE_MEDIA_FILTERING_ELEMENT_H__

#include <list>
#include <string>

#include <whisperlib/common/base/types.h>
#include WHISPER_HASH_MAP_HEADER
#include <whisperlib/net/base/selector.h>

#include <whisperstreamlib/base/element.h>

namespace streaming {

class FilteringElement;

///////////////////////////////////////////////////////////////////////
//
// FilteringCallbackData
// A helper class for FilteringElement - we associate one of this with
// each registered callback.
//
class FilteringCallbackData {
 public:
  FilteringCallbackData();
  virtual ~FilteringCallbackData();

  const string& media_name() const { return media_name_; }
  Tag::Type registered_type() const { return registered_type_; }
  uint32 flavour_mask() const { return flavour_mask_; }

  // Register our processing callback to upstream element
  virtual bool Register(streaming::Request* req);
  virtual bool Unregister(streaming::Request* req);

  struct FilteredTag {
    FilteredTag(const Tag* tag, int64 timestamp_ms) :
      tag_(tag),
      timestamp_ms_(timestamp_ms) {
    }
    scoped_ref<const Tag> tag_;
    int64 timestamp_ms_;
  };
  typedef list<FilteredTag> TagList;

  // Called from ProcessTag.
  // The derived class fills in 'out' with the tags to be sent to the client.
  //  - to drop the 'tag', just leave the 'out' empty.
  //  - to forward the 'tag', insert it into 'out'
  //  - to replace the 'tag': insert the desired Tag s into 'out'.
  // On return, everything in 'out' is forwarded to the client.
  virtual void FilterTag(const Tag* tag, int64 timestamp_ms,  TagList* out) = 0;

  void IncRef() {
    ++ref_count_;
  }
  void DecRef() {
    --ref_count_;
    if ( ref_count_ == 0 ) {
      delete this;
    }
  }

 protected:
  // This is to called affter registration to inform the children about a
  // new particular flavour (flavour has only one bit set :) and give him
  // the possibility to register flavour specific functions..
  // flavour_id in range [0, 31]
  virtual void RegisterFlavour(int flavour_id) {
  }

  // The element that has us
  FilteringElement* master_;
  // Does the registration work
  ElementMapper* mapper_;
  streaming::Request* req_;
  // links to ProcessTag
  ProcessingCallback* here_process_tag_callback_;
  // forward tags to client through this callback (provided by client)
  ProcessingCallback* client_process_tag_callback_;
  // name of the media we're registered to (we suck media from
  // upstream_media_base_/media_name_)
  string media_name_;
  // name of the filtering element we're part of
  string filtering_element_name_;
  // at what kind of stream we are registered
  Tag::Type registered_type_;
  // what flavours are we registered to
  uint32 flavour_mask_;
  // private flags, to be set by subclasses 
  uint32 private_flags_;
  // This is a ref counted object
  int ref_count_;

  // True if an EOS tag was propagated through this
  bool eos_received_;

  // last tag timestamp
  int64 last_tag_ts_;

  // don't append the element's name to the media path
  static const uint32 Flag_DontAppendNameToPath = 0x0001;

 protected:
  // here we receive tags from the element, filter them and forward
  // them to client
  void ProcessTag(const Tag* tag, int64 timestamp_ms);
  // Send a a tag to downstream client.
  void SendTag(const Tag* tag, int64 timestamp_ms);
  // Unregister from upstream, send EOS downstream
  void Close();

  friend class FilteringElement;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(FilteringCallbackData);
};

///////////////////////////////////////////////////////////////////////
//
// FilteringElement
// A element that filters tags by altering, injecting or removing passing
// by tags.
// All the filtering happens in FilteringCallbackData which is a pair of
// callbacks (upstream callback, downstream callback).
//
// To create your own CustomFilteringElement:
//
// class CustomFilteringCallbackData : public FilteringCallbackData {
//   ...
//   virtual void FilterTag(...);
//   ...
// };
// class CustomFilteringElement : public FilteringElement {
//   ...
//   virtual FilteringCallbackData* CreateCallbackData(..) {
//     return new CustomFilteringCallbackData(..);
//   }
//   virtual void DeleteCallbackData(FilteringCallbackData* data) {
//     delete data;
//   }
//   ...
// };
//
class FilteringElement : public Element {
 public:
  FilteringElement(const char* type,
                   const char* name,
                   const char* id,
                   ElementMapper* mapper,
                   net::Selector* selector);
  virtual ~FilteringElement();
 protected:
  // Gives all waiting callbacks an EOS.
  // Does not remove the callbacks! Because that's adder's responsability.
  void CloseAllClients(Closure* call_on_close);

  // Override this to create your own FilteringCallbackData.
  virtual FilteringCallbackData* CreateCallbackData(const char* media_name,
                                                    Request* req) = 0;
  // Destroy a FilteringCallbackData.
  virtual void DeleteCallbackData(FilteringCallbackData* data) {
    data->client_process_tag_callback_ = NULL;
    data->DecRef();
  }

 public:
  // The Element interface methods
  virtual bool Initialize() = 0;
  virtual bool AddRequest(const char* media,
                          streaming::Request* req,
                          streaming::ProcessingCallback* callback);
  virtual void RemoveRequest(streaming::Request* req);

  virtual bool HasMedia(const char* media_name, Capabilities* out);
  virtual void ListMedia(const char* media_dir,
                         ElementDescriptions* media);
  virtual bool DescribeMedia(const string& media,
                             MediaInfoCallback* callback);

  virtual void Close(Closure* call_on_close);

  net::Selector*  selector() const { return selector_; }

 protected:
  net::Selector* const selector_;

  typedef map<Request*, FilteringCallbackData*> FilteringCallbackMap;
  FilteringCallbackMap callbacks_;   // the downstream clients

  // external callback to announce that Close completed.
  Closure* close_completed_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(FilteringElement);
};
}

#endif  // __MEDIA_BASE_MEDIA_FILTERING_ELEMENT_H__
