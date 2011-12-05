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

#ifndef __MEDIA_RTP_RTSP_HEADER_FIELD_H__
#define __MEDIA_RTP_RTSP_HEADER_FIELD_H__

#include <cstdio>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/strutil.h>
#include <whisperlib/common/base/date.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/net/url/url.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_consts.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_common.h>

namespace streaming {
namespace rtsp {

template <typename T>
class OptionalAttribute {
public:
  OptionalAttribute() : is_set_(false) {
  }
  OptionalAttribute(T value) : is_set_(true), value_(value) {
  }
  bool is_set() const {
    return is_set_;
  }
  void set(T value) {
    is_set_ = true;
    value_ = value;
  }
  const OptionalAttribute<T>& operator=(const T& value) {
    set(value);
    return *this;
  }
  const T& get() const {
    return value_;
  }
  bool operator==(const OptionalAttribute<T>& other) const {
    if ( !is_set_ ) {
      return !other.is_set_;
    }
    return value_ == other.value_;
  }
  string ToString() const {
    return is_set() ? strutil::ToString(get()) : string("not-set");
  }
private:
  bool is_set_;
  T value_;
};

class HeaderField {
public:
  enum Type {
    HF_ACCEPT,
    HF_ACCEPT_ENCODING,
    HF_ACCEPT_LANGUAGE,
    HF_ALLOW,
    HF_AUTHORIZATION,
    HF_BANDWIDTH,
    HF_BLOCKSIZE,
    HF_CACHE_CONTROL,
    HF_CONFERENCE,
    HF_CONNECTION,
    HF_CONTENT_BASE,
    HF_CONTENT_ENCODING,
    HF_CONTENT_LANGUAGE,
    HF_CONTENT_LENGTH,
    HF_CONTENT_LOCATION,
    HF_CONTENT_TYPE,
    HF_CSEQ,
    HF_DATE,
    HF_EXPIRES,
    HF_FROM,
    HF_HOST,
    HF_IF_MATCH,
    HF_IF_MODIFIED_SINCE,
    HF_LAST_MODIFIED,
    HF_LOCATION,
    HF_PROXY_AUTHENTICATE,
    HF_PROXY_REQUIRE,
    HF_PUBLIC,
    HF_RANGE,
    HF_REFERER,
    HF_RETRY_AFTER,
    HF_REQUIRE,
    HF_RTP_INFO,
    HF_SCALE,
    HF_SPEED,
    HF_SERVER,
    HF_SESSION,
    HF_TIMESTAMP,
    HF_TRANSPORT,
    HF_UNSUPPORTED,
    HF_USER_AGENT,
    HF_VARY,
    HF_VIA,
    HF_WWW_AUTHENTICATE,
    HF_CUSTOM, // for other than standard fields
  };
  static const char* TypeName(Type type);
  static const string& TypeEnc(Type type);
  static Type TypeDec(const string& type);

  // parse a string of size 'len' or until NULL
  static int ParseInt(const char* s, uint32 len = kMaxUInt32);

public:
  HeaderField(Type type, const string& name)
      : type_(type),
        name_(name) {
  }
  virtual ~HeaderField() {
  }

  const Type type() const {
    return type_;
  }
  const char* type_name() const {
    return TypeName(type());
  }
  const string& name() const {
    return name_;
  }
  virtual bool operator ==(const HeaderField& other) const {
    return type_ == other.type_;
  }

  // just like Encode, but returns the encoded string.
  string StrEncode() const;

  virtual bool Decode(const string& in) = 0;
  virtual void Encode(string* out) const = 0;
private:
  virtual string ToStringBase() const = 0;
public:
  string ToString() const;

private:
  const Type type_;
  const string name_;
};

template <HeaderField::Type T>
class TNumericHeaderField : public HeaderField {
public:
  static const Type kType = T;
  static Type Type() { return kType; }
public:
  TNumericHeaderField()
    : HeaderField(kType, TypeEnc(kType)),
      value_(0) {
  }
  TNumericHeaderField(int32 value)
    : HeaderField(kType, TypeEnc(kType)),
      value_(value) {
  }
  virtual ~TNumericHeaderField() {
  }
  int32 value() const {
    return value_;
  }
  void set_value(int32 value) {
    value_ = value;
  }
  virtual bool operator==(const HeaderField& other) const {
    return HeaderField::operator==(other) &&
           value_ == static_cast<const TNumericHeaderField<T>&>(other).value_;
  }
  virtual bool Decode(const string& in) {
    value_ = HeaderField::ParseInt(in.c_str());
    return true;
  }
  virtual void Encode(string* out) const {
    char tmp[64] = {0, };
    snprintf(tmp, sizeof(tmp), "%d", value_);
    *out = tmp;
  }
private:
  virtual string ToStringBase() const {
    return strutil::StringPrintf("%d", value_);
  }
private:
  int32 value_;
};

/////////////////////////////////////////////////////////////////////////////

class TransportHeaderField : public HeaderField {
public:
  static const Type kType = HF_TRANSPORT;
  static Type Type() { return kType; }
  enum TransmissionType {
    TT_UNICAST,
    TT_MULTICAST,
  };
public:
  TransportHeaderField();
  virtual ~TransportHeaderField();

  const string& transport() const;
  const string& profile() const;
  const OptionalAttribute<string>& lower_transport() const;
  TransmissionType transmission_type() const;
  const OptionalAttribute<string>& destination() const;
  const OptionalAttribute<string>& source() const;
  const OptionalAttribute<int>& layers() const;
  const OptionalAttribute<string>& mode() const;
  bool append() const;
  const OptionalAttribute<pair<int, int> >& interleaved() const;
  const OptionalAttribute<int>& ttl() const;
  const OptionalAttribute<pair<int, int> >& port() const;
  const OptionalAttribute<pair<int, int> >& client_port() const;
  const OptionalAttribute<pair<int, int> >& server_port() const;
  const OptionalAttribute<string>& ssrc() const;

  void set_transport(const string& transport);
  void set_profile(const string& profile);
  void set_lower_transport(const string& lower_transport);
  void set_transmission_type(TransmissionType transmission_type);
  void set_destination(const string& destination);
  void set_source(const string& source);
  void set_layers(const int layers);
  void set_mode(const string& mode);
  void set_append(bool append);
  void set_interleaved(pair<int, int> interleaved);
  void set_ttl(int ttl);
  void set_port(pair<int, int> port);
  void set_client_port(pair<int, int> client_port);
  void set_server_port(pair<int, int> server_port);
  void set_ssrc(const string& ssrc);

  virtual bool operator==(const HeaderField& other) const;

  // e.g. "123-456" => returns pair: (123, 456)
  // On error, returns (0,0)
  static pair<int,int> ParseRange(const string& s);
  // e.g. "abc" => abc
  //      def => def
  static string Unquote(const string& s);

  // convert a range into string format
  // e.g. (123, 456) => "123-456"
  static string StrRange(const pair<int, int>& r);

  virtual bool Decode(const string& in);
  virtual void Encode(string* out) const;
private:
  virtual string ToStringBase() const;
private:
  // e.g. "RTP"
  string transport_;
  // e.g. "AVP"
  string profile_;
  // for RTP/AVP the default is UDP
  OptionalAttribute<string> lower_transport_;
  // default MULTICAST
  TransmissionType transmission_type_;
  // destination address for RTP streaming.
  // Should be identical with client's address, otherwise ignore it as
  // the server may be used as DOS tool.
  OptionalAttribute<string> destination_;
  // Stream source, for server RECORD mode.
  OptionalAttribute<string> source_;
  // the number of multicast layers. Ignore for now.
  OptionalAttribute<int> layers_;
  // "PLAY" or "RECORD".
  // default: "PLAY".
  OptionalAttribute<string> mode_;
  // if mode_ == RECORD, then we append media rather than overwrite.
  // default false
  bool append_;
  // for streaming media through the control channel
  OptionalAttribute<pair<int, int> > interleaved_;
  // multicast time-to-live
  OptionalAttribute<int> ttl_;
  // RTP/RTCP pair for a multicast session
  OptionalAttribute<pair<int, int> > port_;
  // RTP/RTCP pair on client side
  OptionalAttribute<pair<int, int> > client_port_;
  // RTP/RTCP pair on server side
  OptionalAttribute<pair<int, int> > server_port_;
  // synchronization. Ignore for now.
  OptionalAttribute<string> ssrc_;
};

////////////////////////////////////////////////////////////////////////////

class SessionHeaderField : public HeaderField {
public:
  static const Type kType = HF_SESSION;
  static Type Type() { return kType; }
public:
  SessionHeaderField();
  SessionHeaderField(const string& id, uint32 timeout);
  virtual ~SessionHeaderField();

  const string& id() const;
  uint32 timeout() const;
  void set_id(const string& id);
  void set_timeout(uint32 timeout);

  virtual bool operator==(const HeaderField& other) const;

  virtual bool Decode(const string& in);
  virtual void Encode(string* out) const;
private:
  virtual string ToStringBase() const;
private:
  string id_;
  uint32 timeout_;
};

/////////////////////////////////////////////////////////////////////////////

template <HeaderField::Type T>
class TDateHeaderField : public HeaderField {
public:
  static const Type kType = T;
  static Type Type() { return kType; }
public:
  TDateHeaderField()
    : HeaderField(kType, TypeEnc(kType)),
      date_(timer::Date::Now(), true) {
  }
  TDateHeaderField(const timer::Date& date)
    : HeaderField(kType, TypeEnc(kType)),
      date_(date.GetTime(), true) {
  }
  TDateHeaderField(int64 ts)
    : HeaderField(kType, TypeEnc(kType)),
      date_(ts, true) {
  }
  virtual ~TDateHeaderField() {
  }

  virtual bool operator==(const HeaderField& other) const {
    return HeaderField::operator==(other) &&
           date_ == static_cast<const TDateHeaderField<T>&>(other).date_;
  }

  virtual bool Decode(const string& in) {
    const char* sz_in = in.c_str();
    while (!::isdigit(sz_in[0]) && sz_in[0] != 0) {
      sz_in++;
    }
    vector<string> d;
    strutil::SplitString(sz_in, " ", &d);
    if ( d.size() != 5 ) {
      RTSP_LOG_ERROR << type_name() << ": Failed to parse date: [" << in << "]";
      return false;
    }
    int day = ::atoi(d[0].c_str());
    int month = -1;
    static const char* sz_month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sept", "Oct", "Nov", "Dec"};
    for ( int i = 0; i < NUMBEROF(sz_month); i++ ) {
      if ( d[1] == sz_month[i] ) {
        month = i;
        break;
      }
    }
    if ( month == -1 ) {
      RTSP_LOG_ERROR << type_name() << ": Invalid month in date: [" << in << "]";
      return false;
    }
    int year = ::atoi(d[2].c_str());
    if ( d[3].size() != 8 ) {
      RTSP_LOG_ERROR << type_name() << ": Invalid time in date: [" << in << "]";
      return false;
    }
    const int hour = ParseInt(d[3].c_str() + 0, 2);
    const int minute = ParseInt(d[3].c_str() + 3, 2);
    const int second = ParseInt(d[3].c_str() + 6, 2);
    const bool is_gmt = (d[4] == "GMT");
    if ( !is_gmt ) {
      RTSP_LOG_WARNING << type_name() << ": Not a GMT date? in: [" << in << "]";
    }
    date_.Set(year, month, day, hour, minute, second, 0, is_gmt);
    return true;
  }
  virtual void Encode(string* out) const {
    timer::Date d(date_.GetTime(), true);
    static const char* sz_month[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sept", "Oct", "Nov", "Dec"};
    CHECK_GE(d.GetMonth(), 0);
    CHECK_LT(d.GetMonth(), 12);
    *out = strutil::StringPrintf(
        "%d %s %d %02d:%02d:%02d GMT",
        d.GetDay(),
        sz_month[d.GetMonth()],
        d.GetYear(),
        d.GetHour(),
        d.GetMinute(),
        d.GetSecond());
  }
private:
  virtual string ToStringBase() const {
    return date_.ToString();
  }
private:
  timer::Date date_;
};

////////////////////////////////////////////////////////////////////////////

template <HeaderField::Type T>
class TStringHeaderField : public HeaderField {
public:
  static const Type kType = T;
  static Type Type() { return kType; }
public:
  TStringHeaderField()
    : HeaderField(kType, TypeEnc(kType)),
      value_() {
  }
  TStringHeaderField(const string& value)
    : HeaderField(kType, TypeEnc(kType)),
      value_(value) {
  }
  virtual ~TStringHeaderField() {
  }

  const string& value() const {
    return value_;
  }
  void set_value(const string& value) {
    value_ = value;
  }

  virtual bool operator==(const HeaderField& other) const {
    return HeaderField::operator==(other) &&
           value_ == static_cast<const TStringHeaderField<T>&>(other).value_;
  }

  virtual bool Decode(const string& in) {
    value_ = in;
    return true;
  }
  virtual void Encode(string* out) const {
    *out = value_;
  }
private:
  virtual string ToStringBase() const {
    return value_;
  }
private:
  string value_;
};

class CustomHeaderField : public HeaderField {
public:
  static const Type kType = HF_CUSTOM;
  static Type Type() { return kType; }
public:
  CustomHeaderField(const string& name)
    : HeaderField(kType, name),
      value_() {
  }
  CustomHeaderField(const string& name, const string& value)
    : HeaderField(kType, name),
      value_(value) {
  }
  virtual ~CustomHeaderField() {
  }

  const string& value() const {
    return value_;
  }
  void set_value(const string& value) {
    value_ = value;
  }

  virtual bool operator==(const CustomHeaderField& other) const {
    return HeaderField::operator==(other) &&
           value_ == static_cast<const CustomHeaderField&>(other).value_;
  }

  virtual bool Decode(const string& in) {
    value_ = in;
    return true;
  }
  virtual void Encode(string* out) const {
    *out = value_;
  }
private:
  virtual string ToStringBase() const {
    return value_;
  }
private:
  string value_;
};

typedef TNumericHeaderField<HeaderField::HF_CONTENT_LENGTH> ContentLengthHeaderField;
typedef TNumericHeaderField<HeaderField::HF_BANDWIDTH> BandwidthHeaderField;
typedef TNumericHeaderField<HeaderField::HF_BLOCKSIZE> BlocksizeHeaderField;
typedef TNumericHeaderField<HeaderField::HF_CSEQ> CSeqHeaderField;

typedef TDateHeaderField<HeaderField::HF_DATE> DateHeaderField;
typedef TDateHeaderField<HeaderField::HF_EXPIRES> ExpiresHeaderField;
typedef TDateHeaderField<HeaderField::HF_IF_MODIFIED_SINCE> IfModifiedSinceHeaderField;
typedef TDateHeaderField<HeaderField::HF_LAST_MODIFIED> LastModifiedHeaderField;

typedef TStringHeaderField<HeaderField::HF_ACCEPT> AcceptHeaderField;
typedef TStringHeaderField<HeaderField::HF_ACCEPT_ENCODING> AcceptEncodingHeaderField;
typedef TStringHeaderField<HeaderField::HF_ACCEPT_LANGUAGE> AcceptLanguageHeaderField;
typedef TStringHeaderField<HeaderField::HF_ALLOW> AllowHeaderField;
typedef TStringHeaderField<HeaderField::HF_AUTHORIZATION> AuthorizationHeaderField;
typedef TStringHeaderField<HeaderField::HF_CACHE_CONTROL> CacheControlHeaderField;
typedef TStringHeaderField<HeaderField::HF_CONFERENCE> ConferenceHeaderField;
typedef TStringHeaderField<HeaderField::HF_CONNECTION> ConnectionHeaderField;
typedef TStringHeaderField<HeaderField::HF_CONTENT_BASE> ContentBaseHeaderField;
typedef TStringHeaderField<HeaderField::HF_CONTENT_ENCODING> ContentEncodingHeaderField;
typedef TStringHeaderField<HeaderField::HF_CONTENT_LANGUAGE> ContentLanguageHeaderField;
typedef TStringHeaderField<HeaderField::HF_CONTENT_LOCATION> ContentLocationHeaderField;
typedef TStringHeaderField<HeaderField::HF_CONTENT_TYPE> ContentTypeHeaderField;
typedef TStringHeaderField<HeaderField::HF_FROM> FromHeaderField;
typedef TStringHeaderField<HeaderField::HF_HOST> HostHeaderField;
typedef TStringHeaderField<HeaderField::HF_IF_MATCH> IfMatchHeaderField;
typedef TStringHeaderField<HeaderField::HF_LOCATION> LocationHeaderField;
typedef TStringHeaderField<HeaderField::HF_PROXY_AUTHENTICATE> ProxyAuthenticateHeaderField;
typedef TStringHeaderField<HeaderField::HF_PROXY_REQUIRE> ProxyRequireHeaderField;
typedef TStringHeaderField<HeaderField::HF_PUBLIC> PublicHeaderField;
typedef TStringHeaderField<HeaderField::HF_RANGE> RangeHeaderField;
typedef TStringHeaderField<HeaderField::HF_REFERER> RefererHeaderField;
typedef TStringHeaderField<HeaderField::HF_RETRY_AFTER> RetryAfterHeaderField;
typedef TStringHeaderField<HeaderField::HF_REQUIRE> RequireHeaderField;
typedef TStringHeaderField<HeaderField::HF_RTP_INFO> RtpInfoHeaderField;
typedef TStringHeaderField<HeaderField::HF_SCALE> ScaleHeaderField;
typedef TStringHeaderField<HeaderField::HF_SPEED> SpeedHeaderField;
typedef TStringHeaderField<HeaderField::HF_SERVER> ServerHeaderField;
typedef TStringHeaderField<HeaderField::HF_TIMESTAMP> TimestampHeaderField;
typedef TStringHeaderField<HeaderField::HF_UNSUPPORTED> UnsupportedHeaderField;
typedef TStringHeaderField<HeaderField::HF_USER_AGENT> UserAgentHeaderField;
typedef TStringHeaderField<HeaderField::HF_VARY> VaryHeaderField;
typedef TStringHeaderField<HeaderField::HF_VIA> ViaHeaderField;
typedef TStringHeaderField<HeaderField::HF_WWW_AUTHENTICATE> WwwAuthenticateHeaderField;

} // namespace rtsp
} // namespace streaming

ostream& operator<<(ostream& os, const streaming::rtsp::HeaderField& field);

#endif // __MEDIA_RTP_RTSP_HEADER_FIELD_H__
