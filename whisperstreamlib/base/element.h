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

#ifndef __MEDIA_BASE_ELEMENT_H__
#define __MEDIA_BASE_ELEMENT_H__

#include <string>
#include <utility>
#include <vector>

#include <whisperlib/common/base/callback.h>

#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/request.h>
#include <whisperstreamlib/base/element_mapper.h>
#include <whisperstreamlib/base/media_info.h>


namespace streaming {

class Element {
 public:
  explicit Element(const char* type,
                   const char* name,
                   const char* id,
                   ElementMapper* mapper)
      : type_(type), name_(name), id_(id), mapper_(mapper) {
    LOG_DEBUG << "Creating element: " << name_ << " of type: " << type_;
  }
  explicit Element(const char* type,
                   const string& name,
                   const string& id,
                   ElementMapper* mapper)
      : type_(type), name_(name), id_(id), mapper_(mapper) {
    LOG_DEBUG << "Creating element: " << name_ << " of type: " << type_;
  }
  virtual ~Element() {
    LOG_DEBUG << "Deleting element: " << name_ << " of type: " << type_;
  }

  // After creation, the element can initialize itself (like register
  // some callbacks of its own, etc..
  // Return the success status.
  virtual bool Initialize() = 0;

  // Informs the element to include a new client for the tgs
  //
  // media = the name of the media you wish to connect to
  //         (possible w/ parmeters)
  // req   = things associated with the stream request
  //           req.caps() - the kind of things you expect from the
  //           stream ..
  //               - use UNKNOWN_STREAM_TYPE for req.caps().tag_typpe
  //                 for any kind
  // callback = when addition is succesful, it gets called for all tags
  //            in the stream.
  //            ***ESSENTIAL** condition - the callback should be permanent
  //
  // ** YOU MUST ** remove the association between the request and callback
  //                when receiving a TYPE_STREAM_EOS through the callback.
  // and **NEVER**  remove the association between the request and callback
  //                on tag processing unless it is a TYPE_STREAM_EOS !!
  //                (else, strange things may happend..)
  //
  // Important: No callback call happens before the end of the AddRequest
  //            call
  //
  // return:
  //   bool - error - we cannot provide that
  //   * we also return   the type of tags that would mainly come through
  //     that callback
  //       (though you may get some other types too - especially control :)
  //     we update the req->caps() caps to match whatever we can provide.
  virtual bool AddRequest(const char* media,
                          streaming::Request* req,
                          streaming::ProcessingCallback* callback) = 0;
  virtual void RemoveRequest(streaming::Request* req) = 0;

  // test if this element contains the specified media name.
  // On success: returns true, and 'out' is filled with media capabilities.
  // On failure: returns false, 'out' is empty.
  virtual bool HasMedia(const char* media_name, Capabilities* out) = 0;
  // list the name of the contained medias (under the given media path)
  virtual void ListMedia(const char* media_dir,
                         ElementDescriptions* medias) = 0;

  // asynchronously returns media description through the given 'callback'.
  typedef Callback1<const MediaInfo*> MediaInfoCallback;
  virtual bool DescribeMedia(const string& media,
                             MediaInfoCallback* callback) = 0;


  // Closes the element (removing all reqests associated with it .. )
  // As this is an asynchronous operation, two things should happen:
  //  1/ upon return of this function all requests registered w/ this
  //     protocol should be informed of an EOS, and all registered
  //     requests should be considerred removed.
  //  2/ upon the total close of the element, the provided callback should
  //     be called.
  virtual void Close(Closure* call_on_close) = 0;

  // Returns the current config for this element - as a json string,
  // from which an element can be created by its element library.
  virtual string GetElementConfig() {
    return "";    // TODO: make it abstract, implement it for all
  }

  const string& type() const { return type_; }
  const string& name() const { return name_; }
  const string& id() const { return id_; }
 protected:
  const string type_;
  const string name_;
  const string id_;
  ElementMapper* mapper_;

  DISALLOW_EVIL_CONSTRUCTORS(Element);
};

class PolicyDrivenElement;

// A policy stands by and commands a PolicyDrivenElement.
class Policy {
 public:
  Policy(const char* type, const char* name, PolicyDrivenElement* element);
  virtual ~Policy() {
  }
  // Initializes any necessary scheduling etc.
  virtual bool Initialize() = 0;

  // Resets the playlist to an initial (unplayed) state.
  virtual void Reset() = 0;

  // The element notifies the policy about EOS.
  // The policy has 2 options:
  //   - either: element_->SwitchCurrentMedia(..)
  //             return true // element does nothing, already switched
  //   - or: return false // stop element
  virtual bool NotifyEos() = 0;

  // Notifications for the tags required to be processed
  // return: true => forward tag
  //         false => drop tag
  virtual bool NotifyTag(const Tag* tag, int64 timestamp_ms) = 0;

  // Returns the current config for this element - as a json string,
  // from which an element can be created by its element library.
  // Default: return ""
  virtual string GetPolicyConfig() = 0;

  const string& type() const { return type_; }
  const string& name() const { return name_; }
  const PolicyDrivenElement* element() const { return element_; }

 protected:
  const string type_;
  const string name_;
  PolicyDrivenElement* element_;
 private:
  DISALLOW_EVIL_CONSTRUCTORS(Policy);
};

//
// Interface for an element that can be determined of what to serve under
// some media name. The "driving force" is probably a Policy.
// One can imagine a simple application such a source that serves
// a playlist under different names, each playlist is determined by
// a policy (for example). Upon EOS for a media (say 'fileX.mp3') served
// under a media name (say 'playlist 1'), the policy is informed via
// the function registered NotifyEos, and decides to start
// playing something else (say 'fileY.mp3', the next file in playlist).
//
class PolicyDrivenElement : public Element {
 public:
  PolicyDrivenElement(const char* type,
                      const char* name,
                      const char* id,
                      ElementMapper* mapper)
      : Element(type, name, id, mapper) {
  }
  PolicyDrivenElement(const char* type,
                      const string& name,
                      const string& id,
                      ElementMapper* mapper)
      : Element(type, name, id, mapper) {
  }

  // Usually called by the policy, to set to serve under a media name
  // in the driven source
  virtual bool SwitchCurrentMedia(const string& media_name,
                                  const RequestInfo* req_info,
                                  bool force_switch) = 0;
  virtual const string& current_media() const = 0;

  // Basically a way to interrogate the underneath media mapper
  bool HasElementMedia(const char* media_name, Capabilities* out) {
    return mapper_->HasMedia(media_name, out);
  }
  void ListElementMedia(const char* media_dir,
                        ElementDescriptions* media) {
    mapper_->ListMedia(media_dir, media);
  }
  const Policy* policy() const    { return policy_; }
  void set_policy(Policy* policy) {  policy_ = policy; }
 protected:
  Policy* policy_;
 private:
  DISALLOW_EVIL_CONSTRUCTORS(PolicyDrivenElement);
};

inline Policy::Policy(const char* type,
                      const char* name,
                      PolicyDrivenElement* element)
    : type_(type),
      name_(name),
      element_(element) {
  element_->set_policy(this);
}

}

#endif  // __MEDIA_BASE_ELEMENT_H__
