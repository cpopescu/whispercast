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
// A media request contains all data related to a request to
// a media resource (and its resolve).
//

#ifndef __MEDIA_BASE_REQUEST_H__
#define __MEDIA_BASE_REQUEST_H__

#include <string>
#include <vector>
#include <utility>

#include <whisperlib/common/base/types.h>
#include WHISPER_HASH_MAP_HEADER
#include <whisperlib/common/base/callback.h>
#include <whisperlib/common/base/ref_counted.h>
#include <whisperlib/common/base/timer.h>
#include <whisperlib/net/url/url.h>
#include <whisperlib/net/base/user_authenticator.h>
#include <whisperstreamlib/base/consts.h>
#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/stream_auth.h>
#include <whisperstreamlib/base/auto/request_types.h>

namespace streaming {

// Parameters that we recognize in the URL query path:

// Seek position in a stream (most probably a file to be streamed)
// in miliseconds.
static const char kMediaUrlParam_SeekPos[] = "wsp";
// Sets the media origin on the stream
// in miliseconds.
static const char kMediaUrlParam_MediaOrigin[] = "wmo";
// Limits the stream to this timestamp. (E.g. seek_pos=30000&limit=35000
// will most probably cause the streaming of a file from second 30 to second
// 35).
static const char kMediaUrlParam_Limit[] = "wl";
// You can use this to specify the affiliate id (i.e. cookie / other stuff)
// for a media request
static const char kMediaUrlParam_Session[] = "wsi";
// You can use this to specify the affiliate id (i.e. cookie / other stuff)
// for a media request
static const char kMediaUrlParam_Affiliate[] = "wai";
// You can use this to specify the client id (i.e. cookie / other stuff)
// for a media request
static const char kMediaUrlParam_Client[] = "wci";
// Use this to specify the command parameter when publishing
// (if any "save" / "append"
static const char kMediaUrlParam_PublishCommand[] = "wpublish";
// Use this to specify the username in an url (not recommended)
static const char kMediaUrlParam_UserName[] = "wuname";
// Use this to specify the password in an url (not recommended)
static const char kMediaUrlParam_UserPass[] = "wword";
// Use this to specify an authentication token in an url (recommended)
static const char kMediaUrlParam_UserToken[] = "wtoken";
// Use this to set a specific flavour in in the url
static const char kMediaUrlParam_FlavourMask[] = "wfl";


//////////////////////////////

// Viewing action
static const char kActionView[] = "view";
// Publishing action
static const char kActionPublish[] = "publish";


// Other parameters:
// Used internally for remote ip paramtrization
static const char kMediaParam_RemoteIp[] = "remote_ip";
// Used internally for local port paramtrization
static const char kMediaParam_LocalPort[] = "local_port";

//////////////////////////////////////////////////////////////////////

// Capabilities of a media source / request
struct Capabilities {
  uint32 flavour_mask_;
  // TODO(cpopescu): add more ..

  explicit Capabilities()
      : flavour_mask_(kDefaultFlavourMask) {
  }
  explicit Capabilities(uint32 flavour_mask)
      : flavour_mask_(flavour_mask) {
  }
  Capabilities(const Capabilities& caps)
      : flavour_mask_(caps.flavour_mask_) {
  }

  bool IsCompatible(const Capabilities& c) const {
    Capabilities tmp(flavour_mask_);
    tmp.IntersectCaps(c);
    return !tmp.is_invalid();
  }
  bool is_invalid() const {
    return flavour_mask_ == 0;
  }
  void IntersectCaps(const Capabilities& c) {
    flavour_mask_ &= c.flavour_mask_;
  }
  string ToString() const {
    return strutil::StringPrintf("caps[mask: %x]", flavour_mask_);
  }
};

//////////////////////////////////////////////////////////////////////

// Things that we know from the user and are associated with the request
struct RequestInfo {
  // These are parameters for a request
  int64 seek_pos_ms_;         // must be >= 0
  int64 media_origin_pos_ms_; // must be >= 0
  int64 limit_ms_;            // -1: disabled, >= 0: enabled

  // Data about requestor
  string session_id_;                    // e.g. cookie / user id etc
  string affiliate_id_;                  // e.g. cookie / user id etc
  string client_id_;                     // e.g. cookie / user id etc
  string user_agent_;
  net::HostPort remote_address_;
  net::HostPort local_address_;
  int ip_class_;                         // IP classes - per classifier..
  string path_;

  AuthorizerRequest auth_req_;           // requestors's credentials

  int64 write_ahead_ms_;
  // Internal requestor (i.e. a saver);
  bool is_internal_;
  // The requestor is a temporary element.
  // Usefulness: because TimeSavingElement is overused, it needs to
  //             distinguish temporary elem request and NOT save them.
  bool is_temporary_requestor_;

  // Anything goes constructor :)
  RequestInfo()
      : seek_pos_ms_(0),
        media_origin_pos_ms_(0),
        limit_ms_(-1), // undefined
        ip_class_(-1),
        write_ahead_ms_(0),
        is_internal_(false),
        is_temporary_requestor_(false) {
  }

  // Extracts from a url the parameters that can be included in the request
  // info.
  RequestInfo(const RequestInfo& info)
      : seek_pos_ms_(info.seek_pos_ms_),
        media_origin_pos_ms_(info.media_origin_pos_ms_),
        limit_ms_(info.limit_ms_),
        session_id_(info.session_id_),
        affiliate_id_(info.affiliate_id_),
        client_id_(info.client_id_),
        user_agent_(info.user_agent_),
        remote_address_(info.remote_address_),
        local_address_(info.local_address_),
        ip_class_(info.ip_class_),
        path_(info.path_),
        auth_req_(info.auth_req_),
        write_ahead_ms_(info.write_ahead_ms_),
        is_internal_(info.is_internal_),
        is_temporary_requestor_(info.is_temporary_requestor_) {
  }
  // Generates the query part for a url, from curent members
  string GetUrlQuery(bool append_auth) const;

  // Generates the path and query part for a url, from curent members
  string GetUrlPath(const char* media_spec,
                    bool append_auth) const;

  // Generates a 'unique' id string for these capabilities
  string GetId() const;

  // Generates a 'unique' path component to include client, session
  // and affiliate id, as <session_id>/<affiliate_id>/<client_id>
  // If one of them is not set, uses "unset"
  string GetPathId() const;

  string ToString() const;

  void ToSpec(MediaRequestInfoSpec* spec,
              bool save_auth = false) const;
  void FromSpec(const MediaRequestInfoSpec& spec);
};

//
// Information about how to serve a request
struct RequestServingInfo {
  string export_path_;
  string media_name_;
  vector< pair<string, string> > extra_headers_;
  MediaFormat media_format_;
  string content_type_;
  string authorizer_name_;
  int64 flow_control_total_ms_;
  int64 flow_control_video_ms_;
  int64 offset_;
  int64 size_;
  uint32 flavour_mask_;
  bool flavour_mask_is_set_;
  int32 max_clients_;

 public:
  RequestServingInfo()
      : media_format_(kAnyMediaFormat),
        flow_control_total_ms_(0),
        flow_control_video_ms_(0),
        offset_(-1),
        size_(-1),
        flavour_mask_(kDefaultFlavourMask),
        flavour_mask_is_set_(false),
        max_clients_(-1) {
  }
  ~RequestServingInfo() {
  }

  string ToString() const;

  void FromElementExportSpec(const ElementExportSpec& spec);
  void ToElementExportSpec(const string& media_key,
                         ElementExportSpec* spec) const;
};

//////////////////////////////////////////////////////////////////////

class Element;
class ElementController;
class Policy;

typedef hash_map<string, streaming::Element*>     ElementMap;
typedef hash_map<streaming::Element*,
                 vector<streaming::Policy*>*>     PolicyMap;

struct TempElementStruct {
  static const int kDefaultTempElementSize = 6;
  string element_name_;
  ElementMap elements_;
  PolicyMap policies_;

  // external callback, called when all elements were closed
  Closure* close_completed_;

  TempElementStruct()
      : elements_(kDefaultTempElementSize),
        policies_(kDefaultTempElementSize),
        close_completed_(NULL) {
  }
  ~TempElementStruct();
  // Close everything, then call close_completed
  void Close(Closure* close_completed);
  // Called by each element, upon close completion.
  void ElementCloseCompleted(Element* element);
  string ToString() const;
};

//////////////////////////////////////////////////////////////////////

// Type of a callback for processing media tags generated by (or passing
// through) an element
typedef Callback2<const Tag*, int64> ProcessingCallback;

// Things that follow a media request everywhere..
class Request {
  static const int kDefaultCallbackSize = 10;  // so we don't use memory
 public:
  typedef hash_map<ProcessingCallback*, streaming::Element*> CallbacksMap;
  typedef hash_map<streaming::Element*, ProcessingCallback*> RevCallbacksMap;
  Request()
      : callbacks_(kDefaultCallbackSize),
        rev_callbacks_(kDefaultCallbackSize),
        controller_(NULL),
        deleted_(false) {
  }
  Request(const URL& url)
      : callbacks_(kDefaultCallbackSize),
        controller_(NULL),
        deleted_(false) {
    SetFromUrl(url);
  }
  ~Request() {
    CHECK(callbacks_.empty());
  }


  const Capabilities& caps() const                { return caps_; }
  const RequestInfo& info() const                 { return info_; }
  const CallbacksMap& callbacks() const           { return callbacks_; }
  const RevCallbacksMap& rev_callbacks() const    { return rev_callbacks_; }
  const RequestServingInfo& serving_info() const  { return serving_info_; }
  const TempElementStruct& temp_struct() const    { return temp_struct_; }

  Capabilities* mutable_caps()                    { return &caps_; }
  CallbacksMap* mutable_callbacks()               { return &callbacks_; }
  RevCallbacksMap* mutable_rev_callbacks()        { return &rev_callbacks_; }
  RequestInfo* mutable_info()                     { return &info_; }
  RequestServingInfo* mutable_serving_info()      { return &serving_info_; }
  TempElementStruct* mutable_temp_struct()        { return &temp_struct_; }

  void set_controller(ElementController* controller) {
    DCHECK(controller == NULL || controller_ == NULL)
        << " controller_: " << controller_ << ", controller: " << controller;
    controller_ = controller;
  }
  ElementController* controller() { return controller_; }

  string ToString() const;

  void SetFromUrl(const URL& url);

  // Close everything, then asynchronously call close_completed.
  void Close(Closure* close_completed);

 private:
  Capabilities caps_;         // Information about the media
  RequestInfo info_;          // Information about the user request
  RequestServingInfo serving_info_;
                              // Information about how to serve a request
  CallbacksMap callbacks_;    // The processing elements
  RevCallbacksMap rev_callbacks_;  // the reverse of callbacks

  TempElementStruct temp_struct_;

  ElementController* controller_;

 public:
  bool deleted_;

  // TODO: add statistics
 private:
  DISALLOW_EVIL_CONSTRUCTORS(Request);
};

}

inline ostream& operator<<(ostream& os, const streaming::Request& req) {
  return os << req.ToString();
}
inline ostream& operator<<(ostream& os, const streaming::Capabilities& caps) {
  return os << caps.ToString();
}

#endif  //  __MEDIA_BASE_REQUEST_H__
