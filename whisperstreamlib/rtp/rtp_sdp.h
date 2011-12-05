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

#ifndef __MEDIA_RTP_SDP_H__
#define __MEDIA_RTP_SDP_H__

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/io/input_stream.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/net/base/address.h>

namespace streaming {
namespace rtp {

// TODO(cosmin): find a suitable place for this generic function
// Returns network name for the local machine.
// If the machine has no name, returns "".
string LocalMachineName();

class Sdp {
public:
  struct Media {
    // true=audio, false=video
    bool audio_;
    // destination port where the server pushes RTP stream
    uint16 port_;
    // dynamic=96-127, refer to rfc3551
    uint8 avp_payload_type_;
    // sample rate for audio / clock rate for video
    uint32 clock_rate_;
    // a= (session attributes)
    vector<string> attributes_;
    Media();
    Media(bool audio, uint16 port, uint8 avp_payload_type,
          uint32 clock_rate, const vector<string>& attributes);
    string ToString() const;
  };
public:
  Sdp();
  virtual ~Sdp();

  const string& session_name() const;
  const string& session_information() const;
  const string& url() const;
  const string& email() const;
  const string& phone() const;
  const net::IpAddress& local_addr() const;
  const net::IpAddress& remote_addr() const;
  const Media* media(bool audio) const;
  Media* mutable_media(bool audio);

  void set_session_name(const string& session_name);
  void set_session_information(const string& session_information);
  void set_url(const string& url);
  void set_email(const string& email);
  void set_phone(const string& phone);
  void set_local_addr(const net::IpAddress& local_addr);
  void set_remote_addr(const net::IpAddress& remote_addr);

  void add_attribute(const string& attribute);

  void AddMedia(const Media& media);
  // adds an empty media, and returns a pointer to it
  Media* AddMedia();

  bool ReadFile(const string& filename);
  bool ReadStream(io::InputStream& in);
  bool ReadStream(io::MemoryStream& in);
  bool ReadString(const string& str);

  void WriteFile(const string& out_filename) const;
  void WriteStream(io::MemoryStream* out) const;
  string WriteString() const;

private:
  bool ParseLine(const string& line);

public:
  string ToString() const;

private:
  // s= (session name as plain text)
  string session_name_;
  // i= (session plain text description)
  string session_information_;
  // u= (URI of description)
  string url_;
  // e= (email address)
  string email_;
  // p= (phone number)
  string phone_;
  // o= (local address, just to describe the originator of the session)
  net::IpAddress local_addr_;
  // c= (destination address, where we send the RTP stream)
  net::IpAddress remote_addr_;
  // custom attributes
  vector<string> attributes_;

  // The list of media. Each element includes:
  // m= (media name and transport address)
  // b= (bandwidth information)
  // a= (zero or more media attribute lines)
  vector<Media> media_;

  // indicates current media being parsed
  // points to a Media structure inside 'media_' vector
  Media* parser_crt_media_;
};
inline ostream& operator<<(ostream& os, const Sdp::Media& media) {
  return os << media.ToString();
}

}
}

#endif
