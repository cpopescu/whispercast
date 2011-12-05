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

#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/date.h>
#include <whisperlib/common/io/file/file.h>
#include <whisperlib/common/io/file/file_input_stream.h>
#include <whisperlib/common/io/file/file_output_stream.h>
#include <whisperstreamlib/rtp/rtp_sdp.h>
#include <whisperstreamlib/rtp/rtp_util.h>
#include <whisperstreamlib/rtp/rtp_consts.h>

namespace streaming {
namespace rtp {

Sdp::Media::Media(bool audio, uint16 port, uint8 avp_payload_type,
                  uint32 clock_rate, const vector<string>& attributes)
  : audio_(audio),
    port_(port),
    avp_payload_type_(avp_payload_type),
    clock_rate_(clock_rate),
    attributes_(attributes) {
}
Sdp::Media::Media()
  : audio_(true),
    port_(0),
    avp_payload_type_(0),
    clock_rate_(0),
    attributes_() {
}
string Sdp::Media::ToString() const {
  return strutil::StringPrintf(
      "Media{%s, port_: %d, avp_payload_type: %d(%s), clock_rate: %d"
      ", attributes: %s}",
      (audio_ ? "AUDIO" : "VIDEO"),
      port_,
      avp_payload_type_,
      AvpPayloadTypeName(avp_payload_type_),
      clock_rate_,
      strutil::ToString(attributes_).c_str());
}

Sdp::Sdp()
  : session_name_("Unnamed"),
    session_information_("N/A"),
    url_(),
    email_(),
    phone_(),
    local_addr_(),
    remote_addr_(),
    media_(){
}

Sdp::~Sdp() {
}

const string& Sdp::session_name() const {
  return session_name_;
}
const string& Sdp::session_information() const {
  return session_information_;
}
const string& Sdp::url() const {
  return url_;
}
const string& Sdp::email() const {
  return email_;
}
const string& Sdp::phone() const {
  return phone_;
}
const net::IpAddress& Sdp::local_addr() const {
  return local_addr_;
}
const net::IpAddress& Sdp::remote_addr() const {
  return remote_addr_;
}
const Sdp::Media* Sdp::media(bool audio) const {
  for ( uint32 i = 0; i < media_.size(); i++ ) {
    if ( media_[i].audio_ == audio ) {
      return &media_[i];
    }
  }
  return NULL;
}
Sdp::Media* Sdp::mutable_media(bool audio) {
  for ( uint32 i = 0; i < media_.size(); i++ ) {
    if ( media_[i].audio_ == audio ) {
      return &media_[i];
    }
  }
  return NULL;
}
void Sdp::set_session_name(const string& session_name) {
  session_name_ = session_name;
}
void Sdp::set_session_information(const string& session_information) {
  session_information_ = session_information;
}
void Sdp::set_url(const string& url) {
  url_ = url;
}
void Sdp::set_email(const string& email) {
  email_ = email;
}
void Sdp::set_phone(const string& phone) {
  phone_ = phone;
}
void Sdp::set_local_addr(const net::IpAddress& local_addr) {
  local_addr_ = local_addr;
}
void Sdp::set_remote_addr(const net::IpAddress& remote_addr) {
  remote_addr_ = remote_addr;
}
void Sdp::add_attribute(const string& attribute) {
  attributes_.push_back(attribute);
}

void Sdp::AddMedia(const Sdp::Media& media) {
  media_.push_back(media);
}
Sdp::Media* Sdp::AddMedia() {
  media_.push_back(Media());
  return &media_.back();
}

bool Sdp::ReadFile(const string& filename) {
  io::File* f = new io::File();
  if ( !f->Open(filename.c_str(), io::File::GENERIC_READ,
                io::File::OPEN_EXISTING) ) {
    RTP_LOG_ERROR << "Failed to open file: [" << filename << "]";
    delete f;
    return false;
  }
  io::FileInputStream fis(f);
  return ReadStream(fis);
}
bool Sdp::ReadStream(io::InputStream& in) {
  while ( true ) {
    string line;
    if ( !in.ReadLine(&line) ) {
      break;
    }
    if ( !ParseLine(line) ) {
      RTP_LOG_ERROR << "ParseLine failed, for line: [" << line << "]";
      return false;
    }
  }
  return true;
}
bool Sdp::ReadStream(io::MemoryStream& in) {
  while ( true ) {
    string line;
    if ( !in.ReadLFLine(&line) ) {
      break;
    }
    strutil::StrTrimCRLFInPlace(line);
    if ( !ParseLine(line) ) {
      RTP_LOG_ERROR << "ParseLine failed, for line: [" << line << "]";
      return false;
    }
  }
  return true;
}
bool Sdp::ReadString(const string& str) {
  vector<string> v;
  strutil::SplitString(str, "\x0a", &v);
  for ( uint32 i = 0; i < v.size(); i++ ) {
    strutil::StrTrimCRLFInPlace(v[i]); // trim CR 0x0d
    if ( !ParseLine(v[i]) ) {
      RTP_LOG_ERROR << "ParseLine failed, for line: [" << v[i] << "]";
      return false;
    }
  }
  return true;
}

void Sdp::WriteFile(const string& out_filename) const {
  io::File f;
  if ( !f.Open(out_filename.c_str(), io::File::GENERIC_READ_WRITE,
               io::File::CREATE_ALWAYS) ) {
    RTP_LOG_ERROR << "Failed to open file: [" << out_filename << "]"
                 ", err: " << GetLastSystemErrorDescription();
    return;
  }
  string data = WriteString();
  f.Write(data.c_str(), data.size());
  f.Close();
}
void Sdp::WriteStream(io::MemoryStream* out) const {
  string data = WriteString();
  out->Write(data.c_str(), data.size());
}
string Sdp::WriteString() const {
  const int64 now = timer::Date::Now();
  ostringstream oss;
  oss << "v=0" << endl;
  oss << "o=- " << now << " " << now << " IN IP4 "
      << local_addr_.ToString() << endl;
  oss << "s=" << session_name_ << endl;
  oss << "i=" << session_information_ << endl;
  oss << "c=IN " << (remote_addr_.is_ipv4() ? "IP4" : "IP6") << " "
      << remote_addr_.ToString() << endl;
  if ( !url_.empty() ) {
    oss << "u=" << url_ << endl;
  }
  if ( !email_.empty() ) {
    oss << "e=" << email_ << endl;
  }
  if ( !phone_.empty() ) {
    oss << "p=" << phone_ << endl;
  }
  // time the session is active
  oss << "t=0 0" << endl;
  oss << "a=tool:whisperstreamlib 0.1" << endl;
  //oss << "a=recvonly" << endl;
  //oss << "a=type:broadcast" << endl;
  //oss << "a=charset:UTF-8" << endl;
  for ( uint32 i = 0; i < attributes_.size(); i++ ) {
    oss << "a=" << attributes_[i] << endl;
  }
  for ( uint32 i = 0; i < media_.size(); i++ ) {
    const Media& media = media_[i];
    oss << "m=" << (media.audio_ ? "audio" : "video") << " " << media.port_
        << " RTP/AVP " << (uint32)(media.avp_payload_type_) << endl;
    //oss << "b=AS:128" << endl;
    //oss << "b=RR:0" << endl;
    for ( uint32 i = 0; i < media.attributes_.size(); i++ ) {
      oss << "a=" << media.attributes_[i] << endl;
    }
  }
  oss << endl;
  return oss.str();
}

bool Sdp::ParseLine(const string& line) {
  if ( line == "" ) {
    return true;
  }
  if ( line.size() < 2 ||
       line[1] != '=' ) {
    RTP_LOG_ERROR << "Malformed line: [" << line << "]";
    return false;
  }
  const char attr_name = line[0];
  const char* attr_value = line.c_str()+2;
  switch ( attr_name ) {
    case 'v':
      // ignore protocol version
      return true;
    case 's':
      session_name_ = attr_value;
      return true;
    case 'i':
      session_information_ = attr_value;
      return true;
    case 'u':
      url_ = attr_value;
      return true;
    case 'e':
      email_ = attr_value;
      return true;
    case 'p':
      phone_ = attr_value;
      return true;
    case 'o':
      // ignore server time
      return true;
    case 'c': {
      // e.g. "IN IP4 127.0.0.1"
      vector<string> v;
      strutil::SplitString(attr_value, " ", &v);
      if ( v.size() != 3 ) {
        RTP_LOG_ERROR << "Invalid 'c' param, expected 3 fields"
                         ", in line: [" << line << "], ignoring";
        return true;
      }
      if ( v[0] != "IN" ) {
        RTP_LOG_ERROR << "In 'c' param, expected 'IN', found: [" << v[0] << "]"
                         ", in line: [" << line << "], ignoring";
        return true;
      }
      if ( v[1] != "IP4" ) {
        RTP_LOG_ERROR << "In 'c' param, expected 'IP4', found: [" << v[1] << "]"
                         ", in line: [" << line << "], ignoring";
        return true;
      }
      remote_addr_ = net::HostPort(v[2].c_str()).ip_object();
      if ( remote_addr_.IsInvalid() && v[2] != "0.0.0.0" ) {
        RTP_LOG_ERROR << "In 'c' param, invalid IP address: [" << v[2] << "]"
                         ", in line: [" << line << "], ignoring";
      }
      return true;
    }
    case 't':
      // ignore time the session is active
      return true;
    case 'a': {
      // custom attribute
      pair<string, string> p = strutil::SplitFirst(attr_value, ":");
      const string& custom_name = p.first;
      const string& custom_value = p.second;
      (void)custom_value;
      if ( custom_name == "tool" ) {
        // ignore SDP library name
        return true;
      }
      if ( custom_name == "range" ) {
        // ignore time range of the media (media duration)
        return true;
      }
      if ( parser_crt_media_ != NULL ) {
        parser_crt_media_->attributes_.push_back(attr_value);
        return true;
      }
      RTP_LOG_ERROR << "Unknown custom attribute: '" << custom_name << "' in"
                       " line: [" << line << "], ignoring..";
      return true;
    }
    case 'm': {
      // e.g. "audio 0 RTP/AVP 96"
      vector<string> v;
      strutil::SplitString(attr_value, " ", &v);
      if ( v.size() != 4 ) {
        RTP_LOG_ERROR << "Invalid 'm' param, expected 4 fields"
                         ", in line: [" << line << "], ignoring";
        return true;
      }
      parser_crt_media_ = AddMedia();
      if ( v[0] == "audio" ) {
        parser_crt_media_->audio_ = true;
      } else if ( v[0] == "video" ) {
        parser_crt_media_->audio_ = false;
      } else {
        RTP_LOG_ERROR << "Invalid 'm' param, expected 'audio'/'video'"
                         ", in line: [" << line << "], ignoring";
        return true;
      }
      parser_crt_media_->port_ = ::atol(v[1].c_str());
      if ( v[2] != "RTP/AVP" ) {
        RTP_LOG_ERROR << "Invalid 'm' param, expected 'RTP/AVP'"
                         ", in line: [" << line << "], ignoring";
        return true;
      }
      parser_crt_media_->avp_payload_type_ = ::atol(v[3].c_str());
      return true;
    }
    default:
      RTP_LOG_ERROR << "Unknown attribute: '" << attr_name << "'"
                   " in line: [" << line << "], ignoring..";
      return true;
  }
}

string Sdp::ToString() const {
  return strutil::StringPrintf("Sdp{\n"
      "session_name_: %s,\n"
      "session_information_: %s,\n"
      "url_: %s,\n"
      "email_: %s,\n"
      "phone_: %s,\n"
      "remote_addr_: %s,\n"
      "media_: %s}",
      session_name_.c_str(),
      session_information_.c_str(),
      url_.c_str(),
      email_.c_str(),
      phone_.c_str(),
      remote_addr_.ToString().c_str(),
      strutil::ToString(media_).c_str());
}


}
}
