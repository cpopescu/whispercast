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

#include <whisperlib/common/base/re.h>
#include <whisperlib/common/base/scoped_ptr.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_decoder.h>
#include <whisperstreamlib/rtp/rtsp/types/rtsp_header_field.h>

#define ILOG_DEBUG   RTSP_LOG_DEBUG << Info()
#define ILOG_INFO    RTSP_LOG_INFO << Info()
#define ILOG_WARNING RTSP_LOG_WARNING << Info()
#define ILOG_ERROR   RTSP_LOG_ERROR << Info()
#define ILOG_FATAL   RTSP_LOG_FATAL << Info()

namespace streaming {
namespace rtsp {

const char* ParserStateName(ParserState state) {
  switch(state) {
    CONSIDER(PARSING_IDLE);
    CONSIDER(PARSING_HEADER);
    CONSIDER(PARSING_BODY);
  }
  LOG_FATAL << "Invalid state: " << state;
  return "Unknown";
}

////////////////////////////////////////////////////////////////////////////

bool Decoder::ParseString(char*& in, char sep) {
  while ( *in != sep && *in != 0 ) {
    in++;
  }
  if ( *in == 0 ) {
    return false;
  }
  // NULLify & skip separator
  *in = 0;
  in++;
  return true;
}
bool Decoder::ParseInteger(char*& in, uint32* out) {
  if ( *in < '0' || *in > '9' ) {
    return false;
  }
  *out = 0;
  while ( *in >= '0' && *in <= '9' ) {
    *out = (*out) * 10 + (*in) - '0';
    in++;
  }
  return true;
}
bool Decoder::ParseExpected(char*& in, const char* expected) {
  while ( *expected != 0 && *in == *expected ) {
    in++;
    expected++;
  }
  return *expected == 0;
}
void Decoder::ParseBlanks(char*& in) {
  while ( *in == ' ' ) {
    in++;
  }
}

bool Decoder::ParseRequestLine(const string& line,
    REQUEST_METHOD* out_method, URL** out_url,
    uint32* out_ver_hi, uint32* out_ver_lo) {
  char* sz_line = ::strdup(line.c_str());
  scoped_ptr<char> auto_del_sz_line(sz_line);

  const char* method = sz_line;
  if ( !ParseString(sz_line, ' ') ) {
    RTSP_LOG_ERROR << "Error decoding method from: [" << line << "]";
    return false;
  }
  char* str_url = sz_line;
  if ( !ParseString(sz_line, ' ') ) {
    RTSP_LOG_ERROR << "Error decoding url from: [" << line << "]";
    return false;
  }
  if ( !ParseExpected(sz_line, "RTSP/") ) {
    RTSP_LOG_ERROR << "Invalid request line: [" << line << "], no 'RTSP/'";
    return false;
  }
  uint32 ver_hi = 0;
  uint32 ver_lo = 0;
  if ( !ParseInteger(sz_line, &ver_hi) ||
       !ParseExpected(sz_line, ".") ||
       !ParseInteger(sz_line, &ver_lo) ) {
    RTSP_LOG_ERROR << "Error decoding version from: [" << line << "]";
    return false;
  }
  if ( sz_line[0] != 0 ) {
    RTSP_LOG_ERROR << "Invalid request line: [" << line << "]"
                       ", extra characters";
    return false;
  }
  ////////////////////////////////////////////////////////////
  // request line parsed
  REQUEST_METHOD met = RequestMethodDec(method);
  if ( met == -1 ) {
    RTSP_LOG_ERROR << "Invalid method in line: [" << line << "]";
    return false;
  }
  if ( ver_hi != kRtspVersionHi || ver_lo != kRtspVersionLo ) {
    RTSP_LOG_WARNING << "Strange RTSP version: " << ver_hi << "." << ver_lo
                     << " in request line: [" << line << "]; server version"
                        " is: " << kRtspVersionHi << "." << kRtspVersionLo;
  }
  URL* url = NULL;
  if ( str_url[0] != '*' ) {
    // URL can have 2 valid formats:
    //  - "rtsp://1.2.3.4/normalizer/sample.f4v" -- seen in VLC player
    //  - "normalizer/sample.f4v"                -- seen in Android video player
    // Or invalid format:
    //  - "http://1.2.3.4/normalizer/sample.f4v" -- HTTP request, not RTSP
    if ( ::strstr(str_url, "://") ) {
      url = new URL(str_url);
    } else {
      url = new URL(string("rtsp://127.0.0.1/") + str_url);
    }
    if ( !url->is_valid() ) {
      RTSP_LOG_ERROR << "Invalid URL: [" << str_url << "]"
                        " in request line: [" << line << "]";
      delete url;
      return false;
    }
    if ( !url->SchemeIs("rtsp") ) {
      RTSP_LOG_ERROR << "Invalid protocol in request line: [" << line << "]";
      return false;
    }
  }

  *out_method = met;
  *out_url = url;
  *out_ver_hi = ver_hi;
  *out_ver_lo = ver_lo;
  return true;
}

bool Decoder::ParseResponseLine(const string& line,
    STATUS_CODE* out_status_code, string* out_description,
    uint32* out_ver_hi, uint32* out_ver_lo) {
  char* sz_line = ::strdup(line.c_str());
  scoped_ptr<char> auto_del_sz_line(sz_line);

  if ( !ParseExpected(sz_line, "RTSP/") ) {
    RTSP_LOG_ERROR << "Invalid response line: [" << line << "], no 'RTSP/'";
    return false;
  }
  uint32 ver_hi = 0;
  uint32 ver_lo = 0;
  if ( !ParseInteger(sz_line, &ver_hi) ||
       !ParseExpected(sz_line, ".") ||
       !ParseInteger(sz_line, &ver_lo) ) {
    RTSP_LOG_ERROR << "Error decoding version from: [" << line << "]";
    return false;
  }
  if ( ver_hi != kRtspVersionHi || ver_lo != kRtspVersionLo ) {
    RTSP_LOG_WARNING << "Strange RTSP version: " << ver_hi << "." << ver_lo
                     << " in response line: [" << line << "]; server version"
                        " is: " << kRtspVersionHi << "." << kRtspVersionLo;
  }
  ParseBlanks(sz_line);
  uint32 status = 0;
  if ( !ParseInteger(sz_line, &status) ) {
    RTSP_LOG_ERROR << "Error decoding status code from: [" << line << "]";
    return false;
  }
  ParseBlanks(sz_line);
  const char* description = sz_line;
  /////////////////////////////////////////////////////////////////////////
  STATUS_CODE status_code = StatusCodeDec(status);
  if ( status_code == -1 ) {
    RTSP_LOG_ERROR << "Invalid status code in line: [" << line << "]";
    return false;
  }
  *out_status_code = status_code;
  *out_description = description;
  *out_ver_hi = ver_hi;
  *out_ver_lo = ver_lo;
  return true;
}

Decoder::Decoder()
  : remote_addr_("NOT SET"),
    msg_(NULL),
    content_length_(0),
    state_(PARSING_IDLE) {
}
Decoder::~Decoder() {
  delete msg_;
  msg_ = NULL;
}

void Decoder::set_remote_address(const string& remote_address) {
  remote_addr_ = remote_address;
}
DecodeStatus Decoder::Decode(io::MemoryStream& in, Message** out) {
  if ( msg_ == NULL ) {
    char tmp[5] = {0,};
    int32 read = in.Peek(tmp, 4);
    if ( read < 4 ) {
      // wait for more data
      return DECODE_NO_DATA;
    }
    msg_ = (tmp[0] == '$'
             ? (Message*)new InterleavedFrame()
             : (::strcmp(tmp, "RTSP") == 0
             ? (Message*)new Response(STATUS_OK)
             : (Message*)new Request(METHOD_DESCRIBE)));
    // reset parser's internal state
    content_length_ = 0;
    state_ = PARSING_IDLE;
  }
  // keep gcc happy w/ initialization
  DecodeStatus status = DECODE_SUCCESS;
  switch(msg_->type()) {
    case Message::MESSAGE_REQUEST:
      status = DecodeRequest(in);
      break;
    case Message::MESSAGE_RESPONSE:
      status = DecodeResponse(in);
      break;
    case Message::MESSAGE_INTERLEAVED_FRAME:
      status = DecodeInterleavedFrame(in);
      break;
  };
  if ( status != DECODE_SUCCESS ) {
    return status;
  }
  *out = msg_;
  msg_ = NULL;
  return DECODE_SUCCESS;
}

DecodeStatus Decoder::DecodeRequest(io::MemoryStream& in) {
  CHECK_EQ(msg_->type(), Message::MESSAGE_REQUEST);
  Request* req = static_cast<Request*>(msg_);
  if ( state_ == PARSING_IDLE ) {
    string line;
    if ( !in.ReadLine(&line) ) {
      ILOG_DEBUG << "No more data for another line";
      return DECODE_NO_DATA;
    }

    REQUEST_METHOD method = METHOD_DESCRIBE;
    URL* url;
    uint32 ver_hi = 0;
    uint32 ver_lo = 0;
    if ( !Decoder::ParseRequestLine(line, &method, &url, &ver_hi, &ver_lo) ) {
      RTSP_LOG_ERROR << "Failed to decode request line: [" << line << "]";
      return DECODE_ERROR;
    }

    req->mutable_header()->set_method(method);
    req->mutable_header()->set_url(url);
    state_ = PARSING_HEADER;
    ILOG_DEBUG << "Request line: method: " << RequestMethodName(method)
               << ", url: [" << (url == NULL ? "*" : url->spec().c_str())
               << "]";
  }
  return DecodeHeaderAndBody(in, req->mutable_header(), req->mutable_data());
}
DecodeStatus Decoder::DecodeResponse(io::MemoryStream& in) {
  CHECK_EQ(msg_->type(), Message::MESSAGE_RESPONSE);
  Response* rsp = static_cast<Response*>(msg_);
  if ( state_ == PARSING_IDLE ) {
    string line;
    if ( !in.ReadLine(&line) ) {
      ILOG_DEBUG << "No more data for another line";
      return DECODE_NO_DATA;
    }

    STATUS_CODE status_code = STATUS_OK;
    string status_description;
    uint32 ver_hi = 0;
    uint32 ver_lo = 0;
    if ( !Decoder::ParseResponseLine(line, &status_code, &status_description,
        &ver_hi, &ver_lo) ) {
      RTSP_LOG_ERROR << "Failed to decode response line: [" << line << "]";
      return DECODE_ERROR;
    }

    rsp->mutable_header()->set_status(status_code);
    state_ = PARSING_HEADER;
    ILOG_DEBUG << "Response line: status_code: " << status_code
               << ", description: [" << status_description << "]";
  }
  return DecodeHeaderAndBody(in, rsp->mutable_header(), rsp->mutable_data());
}
DecodeStatus Decoder::DecodeInterleavedFrame(io::MemoryStream& in) {
  CHECK_EQ(msg_->type(), Message::MESSAGE_INTERLEAVED_FRAME);
  if ( in.Size() < 4 ) {
    ILOG_DEBUG << "Not enough data for InterleavedFrame header";
    return DECODE_NO_DATA;
  }
  InterleavedFrame* f = static_cast<InterleavedFrame*>(msg_);
  // skip the '$'
  in.Skip(1);
  f->set_channel(io::NumStreamer::ReadByte(&in));
  uint16 size = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  if ( in.Size() < size ) {
    ILOG_DEBUG << "Not enough data for InterleavedFrame body";
    return DECODE_NO_DATA;
  }
  f->mutable_data()->AppendStream(&in, size);
  return DECODE_SUCCESS;
}
DecodeStatus Decoder::DecodeHeaderAndBody(io::MemoryStream& in,
    BaseHeader* out_header, io::MemoryStream* out_body) {
  if ( state_ == PARSING_HEADER ) {
    DecodeStatus status = Decoder::DecodeHeader(in, out_header);
    if ( status != DECODE_SUCCESS ) {
      return status;
    }
    // header complete. Now check CSeq and Content-Length.
    if ( out_header->get_field(kHeaderFieldCSeq) == NULL ) {
      ILOG_ERROR << "Header complete, but no CSeq field found.";
      return DECODE_ERROR;
    }
    const ContentLengthHeaderField* content_length =
        out_header->tget_field<ContentLengthHeaderField>();
    if ( content_length != NULL ) {
      content_length_ = content_length->value();
    }
    if ( content_length_ == 0 ) {
      state_ = PARSING_IDLE;
      ILOG_DEBUG << "Header complete, no body expected, response complete.";
      return DECODE_SUCCESS;
    }
    state_ = PARSING_BODY;
  }
  if ( state_ == PARSING_BODY ) {
    CHECK_LT(out_body->Size(), content_length_);
    out_body->AppendStream(&in, content_length_ - out_body->Size());
    ILOG_DEBUG << "Fill in body.. " << out_body->Size()
               << "/" << content_length_ << " bytes. in: " << in.Size();
    CHECK_LE(out_body->Size(), content_length_);
    if ( out_body->Size() == content_length_ ) {
      state_ = PARSING_IDLE;
      ILOG_INFO << "Body complete, response complete.";
      return DECODE_SUCCESS;
    }
    return DECODE_NO_DATA;
  }
  ILOG_FATAL << "Inaccessible line of code";
  return DECODE_ERROR;
}
DecodeStatus Decoder::DecodeHeader(io::MemoryStream& in, BaseHeader* out) {
  while ( true ) {
    string line;
    if ( !in.ReadLine(&line) ) {
      RTSP_LOG_DEBUG << "No more data for another line";
      return DECODE_NO_DATA;
    }
    if ( line == "" ) {
      return DECODE_SUCCESS;
    }

    const char* key_start = line.c_str();
    // trim ' ' at key start
    while ( *key_start == ' ' ) { key_start++; }
    const char* key_end = key_start;
    while ( *key_end != ':' && *key_end != 0 ) { key_end++; }
    if ( *key_end == 0 || key_end == key_start ) {
      ILOG_ERROR << "No key in header line: [" << line << "]";
      return DECODE_ERROR;
    }
    const char* value_start = key_end + 1; // after ':'

    // trim ' ' at key end
    key_end--;
    while ( *key_end == ' ' ) { key_end--; }

    // trim ' ' at value start
    while ( *value_start == ' ' ) { value_start++; }
    if ( *value_start == 0 ) {
      ILOG_ERROR << "No value in header line: [" << line << "] .. ignoring";
    }
    const char* value_end = max(value_start, line.c_str() + line.size() - 1);
    // trim ' ' at value end
    while ( *value_end == ' ' ) { value_end--; }

    const string key(key_start, key_end - key_start + 1);
    const string value(value_start, value_end - value_start + 1);

    HeaderField::Type hf = HeaderField::TypeDec(key);
    HeaderField* field = NULL;
    switch ( hf ) {
      case HeaderField::HF_ACCEPT: field = new AcceptHeaderField(); break;
      case HeaderField::HF_ACCEPT_ENCODING: field = new AcceptEncodingHeaderField(); break;
      case HeaderField::HF_ACCEPT_LANGUAGE: field = new AcceptLanguageHeaderField(); break;
      case HeaderField::HF_ALLOW: field = new AllowHeaderField(); break;
      case HeaderField::HF_AUTHORIZATION: field = new AuthorizationHeaderField(); break;
      case HeaderField::HF_BANDWIDTH: field = new BandwidthHeaderField(); break;
      case HeaderField::HF_BLOCKSIZE: field = new BlocksizeHeaderField(); break;
      case HeaderField::HF_CACHE_CONTROL: field = new CacheControlHeaderField(); break;
      case HeaderField::HF_CONFERENCE: field = new ConferenceHeaderField(); break;
      case HeaderField::HF_CONNECTION: field = new ConnectionHeaderField(); break;
      case HeaderField::HF_CONTENT_BASE: field = new ContentBaseHeaderField(); break;
      case HeaderField::HF_CONTENT_ENCODING: field = new ContentEncodingHeaderField(); break;
      case HeaderField::HF_CONTENT_LANGUAGE: field = new ContentLanguageHeaderField(); break;
      case HeaderField::HF_CONTENT_LENGTH: field = new ContentLengthHeaderField(); break;
      case HeaderField::HF_CONTENT_LOCATION: field = new ContentLocationHeaderField(); break;
      case HeaderField::HF_CONTENT_TYPE: field = new ContentTypeHeaderField(); break;
      case HeaderField::HF_CSEQ: field = new CSeqHeaderField(); break;
      case HeaderField::HF_DATE: field = new DateHeaderField(); break;
      case HeaderField::HF_EXPIRES: field = new ExpiresHeaderField(); break;
      case HeaderField::HF_FROM: field = new FromHeaderField(); break;
      case HeaderField::HF_HOST: field = new HostHeaderField(); break;
      case HeaderField::HF_IF_MATCH: field = new IfMatchHeaderField(); break;
      case HeaderField::HF_IF_MODIFIED_SINCE: field = new IfModifiedSinceHeaderField(); break;
      case HeaderField::HF_LAST_MODIFIED: field = new LastModifiedHeaderField(); break;
      case HeaderField::HF_LOCATION: field = new LocationHeaderField(); break;
      case HeaderField::HF_PROXY_AUTHENTICATE: field = new ProxyAuthenticateHeaderField(); break;
      case HeaderField::HF_PROXY_REQUIRE: field = new ProxyRequireHeaderField(); break;
      case HeaderField::HF_PUBLIC: field = new PublicHeaderField(); break;
      case HeaderField::HF_RANGE: field = new RangeHeaderField(); break;
      case HeaderField::HF_REFERER: field = new RefererHeaderField(); break;
      case HeaderField::HF_RETRY_AFTER: field = new RetryAfterHeaderField(); break;
      case HeaderField::HF_REQUIRE: field = new RequireHeaderField(); break;
      case HeaderField::HF_RTP_INFO: field = new RtpInfoHeaderField(); break;
      case HeaderField::HF_SCALE: field = new ScaleHeaderField(); break;
      case HeaderField::HF_SPEED: field = new SpeedHeaderField(); break;
      case HeaderField::HF_SERVER: field = new ServerHeaderField(); break;
      case HeaderField::HF_SESSION: field = new SessionHeaderField(); break;
      case HeaderField::HF_TIMESTAMP: field = new TimestampHeaderField(); break;
      case HeaderField::HF_TRANSPORT: field = new TransportHeaderField(); break;
      case HeaderField::HF_UNSUPPORTED: field = new UnsupportedHeaderField(); break;
      case HeaderField::HF_USER_AGENT: field = new UserAgentHeaderField(); break;
      case HeaderField::HF_VARY: field = new VaryHeaderField(); break;
      case HeaderField::HF_VIA: field = new ViaHeaderField(); break;
      case HeaderField::HF_WWW_AUTHENTICATE: field = new WwwAuthenticateHeaderField(); break;
      case HeaderField::HF_CUSTOM: field = new CustomHeaderField(key); break;
    }
    if ( field == NULL ) {
      ILOG_ERROR << "Unrecognized header field: [" << line << "]";
      return DECODE_ERROR;
    }
    if ( !field->Decode(value) ) {
      ILOG_ERROR << "Failed to decode header field: [" << line << "]";
      delete field;
      return DECODE_ERROR;
    }
    if ( out->get_field(field->name()) != NULL ) {
      ILOG_ERROR << "Duplicate field, old: ["
                 << out->get_field(field->name())->ToString()
                 << "], new: [" << field->ToString();
      delete field;
      return DECODE_ERROR;
    }
    ILOG_DEBUG << "Found Header Field: " << field->ToString();
    out->add_field(field);
  }
}
string Decoder::Info() const {
  return strutil::StringPrintf("[Decoder %s %s %s] ",
      (msg_ == NULL ? "MESSAGE_NULL" : msg_->type_name()),
      ParserStateName(state_), remote_addr_.c_str());
}

}
}
