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

#ifndef __MEDIA_RTP_RTSP_MESSAGE_H__
#define __MEDIA_RTP_RTSP_MESSAGE_H__

#include <cstdio>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/strutil.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/net/url/url.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_consts.h>
#include <whisperstreamlib/rtp/rtsp/types/rtsp_header.h>

namespace streaming {
namespace rtsp {

class Connection;
class Response;
class Request;

// because we don't include rtsp_connectio.h => we cannot call conn->ToString()
string ConnectionToString(Connection* conn);

class Message {
public:
  enum Type {
    MESSAGE_REQUEST,
    MESSAGE_RESPONSE,
    MESSAGE_INTERLEAVED_FRAME,
  };
  static const char* TypeName(Type type);
public:
  Message(Type type);
  Message(const Message& other);
  virtual ~Message();

  const Type type() const;
  const char* type_name() const;

  virtual bool operator==(const Message& other) const;

  virtual string ToString() const = 0;
private:
  const Type type_;
};

template<typename HEADER>
class HeadedMessage : public Message {
public:
  HeadedMessage(Message::Type type)
    : Message(type),
      header_(),
      data_() {
  }
  HeadedMessage(const HeadedMessage<HEADER>& other)
    : Message(other),
      header_(other.header_),
      data_() {
    data_.AppendStreamNonDestructive(&other.data());
  }
  virtual ~HeadedMessage() {
  }

  const HEADER& header() const {
    return header_;
  }
  HEADER* mutable_header() {
    return &header_;
  }

  const io::MemoryStream& data() const {
    return data_;
  }
  io::MemoryStream* mutable_data() {
    return &data_;
  }

  virtual bool operator==(const Message& other) const {
    if ( !Message::operator==(other) ) {
      return false;
    }
    const HeadedMessage<HEADER>& other_headed_message =
        static_cast<const HeadedMessage<HEADER>&>(other);
    return header_ == other_headed_message.header_ &&
           data_.Equals(other_headed_message.data_);
  }

  // Updates Content-Type and Content-Length fields
  void UpdateContent(const string& content_type) {
    ContentLengthHeaderField* content_length_field =
        static_cast<ContentLengthHeaderField*>(
            mutable_header()->get_field(kHeaderFieldContentLength));
    if ( content_length_field == NULL ) {
      content_length_field = new ContentLengthHeaderField();
      mutable_header()->add_field(content_length_field);
    }
    content_length_field->set_value(data_.Size());

    ContentTypeHeaderField* content_type_field =
        static_cast<ContentTypeHeaderField*>(
            mutable_header()->get_field(kHeaderFieldContentType));
    if ( content_type_field == NULL ) {
      content_type_field = new ContentTypeHeaderField();
      mutable_header()->add_field(content_type_field);
    }
    content_type_field->set_value(content_type);
  }
  // tests if Content-Length is set correctly
  bool CheckContent() const {
    const ContentLengthHeaderField* content_length =
        static_cast<const ContentLengthHeaderField*>(
            header().get_field(kHeaderFieldContentLength));
    if ( content_length == NULL ) {
      return data_.IsEmpty();
    }
    return content_length->value() == data_.Size();
  }

  virtual string ToString() const {
    return strutil::StringPrintf(
        "rtsp::%s{header_: %s, data_: #%d bytes}",
        header_.type() == BaseHeader::HEADER_REQUEST ? "Request" : "Response",
        header_.ToString().c_str(), data_.Size());
  }
protected:
  HEADER header_;
  io::MemoryStream data_;
};

} // namespace rtsp
} // namespace streaming

#endif // __MEDIA_RTP_RTSP_MESSAGE_H__
