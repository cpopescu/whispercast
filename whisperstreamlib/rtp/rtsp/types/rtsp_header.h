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

#ifndef __MEDIA_RTP_RTSP_HEADER_H__
#define __MEDIA_RTP_RTSP_HEADER_H__

#include <cstdio>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/strutil.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/net/url/url.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_consts.h>
#include <whisperstreamlib/rtp/rtsp/types/rtsp_header_field.h>

namespace streaming {
namespace rtsp {

class BaseHeader {
public:
  typedef map<string, HeaderField*> FieldMap;
  enum Type {
    HEADER_REQUEST,
    HEADER_RESPONSE,
  };
public:
  BaseHeader(Type type);
  BaseHeader(const BaseHeader& other);
  virtual ~BaseHeader();

  Type type() const;
  uint8 version_hi() const;
  uint8 version_lo() const;

  void add_field(HeaderField* field);
  const HeaderField* get_field(const string& field_name) const;
  HeaderField* get_field(const string& field_name);
  const FieldMap& fields() const;

  template <typename T>
  const T* tget_field() const {
    const string& field_name = HeaderField::TypeEnc(T::Type());
    FieldMap::const_iterator it = fields_.find(field_name);
    return it == fields_.end() ? NULL : static_cast<const T*>(it->second);
  }
  template <typename T>
  T* tget_field() {
    const string& field_name = HeaderField::TypeEnc(T::Type());
    FieldMap::iterator it = fields_.find(field_name);
    return it == fields_.end() ? NULL : static_cast<T*>(it->second);
  }
  virtual bool operator==(const BaseHeader& other) const;

  string ToStringBase() const;
private:
  const Type type_;
  uint8 version_hi_;
  uint8 version_lo_;
  FieldMap fields_;
};

class RequestHeader : public BaseHeader {
public:
  static const Type kType = HEADER_REQUEST;
public:
  RequestHeader();
  RequestHeader(const RequestHeader& other);
  virtual ~RequestHeader();

  REQUEST_METHOD method() const;
  const URL* url() const;
  // media path from URL, or "*"
  string media() const;

  void set_method(REQUEST_METHOD method);
  void set_url(URL* url);

  virtual bool operator==(const BaseHeader& other) const;

  // Returns url query parameter. Or "".
  string GetQueryParam(const string& name) const;

  string ToString() const;
private:
  REQUEST_METHOD method_;
  // The URL of the request.
  // If NULL then url is '*'.
  URL* url_;
};

class ResponseHeader : public BaseHeader {
public:
  static const Type kType = HEADER_RESPONSE;
public:
  ResponseHeader();
  ResponseHeader(const ResponseHeader& other);
  virtual ~ResponseHeader();

  STATUS_CODE status() const;
  const char* status_name() const;

  void set_status(STATUS_CODE status);

  virtual bool operator==(const BaseHeader& other) const;

  string ToString() const;
private:
  STATUS_CODE status_;
};

} // namespace rtsp
} // namespace streaming

#endif // __MEDIA_RTP_RTSP_HEADER_H__
