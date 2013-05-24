// Copyright (c) 2011, Whispersoft s.r.l.
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

#ifndef __MEDIA_BASE_IMPORTER_H__
#define __MEDIA_BASE_IMPORTER_H__

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/callback/callback1.h>
#include <whisperstreamlib/base/request.h>
#include <whisperstreamlib/base/media_info.h>

namespace streaming {

// The Publisher is usually the Network Protocol
class Publisher {
 public:
  Publisher() {}
  virtual ~Publisher() {}
  virtual void StartCompleted(bool success) = 0;
  virtual void Stop() = 0;
  virtual const MediaInfo& GetMediaInfo() const = 0;
};

// The importer is usually a whispercast internal Element
// Tags circulate from network => Publisher => Importer => inside whispercast
class Importer {
 public:
  enum Type {
    TYPE_RTMP,
    TYPE_HTTP,
  };
  static const char* TypeName(Type type) {
    switch ( type ) {
      CONSIDER(TYPE_RTMP);
      CONSIDER(TYPE_HTTP);
    }
    LOG_FATAL << "Illegal type: " << type;
  }
 public:
  Importer(Type type)
    : type_(type) {
  }
  virtual ~Importer() {
  }
  Type type() const {
    return type_;
  }
  const char* type_name() const {
    return TypeName(type());
  }

  // The path this importer listens on.
  virtual string ImportPath() const = 0;

  // Incoming Tag from publisher
  virtual void ProcessTag(const Tag* tag, int64 timestamp_ms) = 0;

 private:
  Type type_;
};

class RtmpImporter : public Importer {
 public:
  RtmpImporter() : Importer(Importer::TYPE_RTMP) {}
  virtual ~RtmpImporter() {
  }

  //  Asynchronously starts the RMTP publishing.
  // Params:
  //  subpath: path inside the importer
  //  auth: authorization request (user,pass,token,..)
  //  command: can be "live", "record" or "append"
  //  publisher: The rtmp::PublishStream which is going to send us tags
  // When Start() completes, we call publisher->StartCompleted().
  virtual void Start(string subpath,
                     AuthorizerRequest auth,
                     string command,
                     Publisher* publisher) = 0;
  // If the publisher wishes to stop the importer: it sends EosTag.
  // If the importer wishes to stop the publisher: it calls publisher->Stop().
};
class HttpImporter : public Importer {
 public:
  HttpImporter() : Importer(Importer::TYPE_HTTP) {}
  virtual ~HttpImporter() {
  }

  //  Asynchronous start importing.
  // Params:
  //  subpath: path inside the importer
  //  auth: authorization request (user,pass,token,..)
  //  publisher: The http::StreamRequest which is going to send us tags
  //virtual void Start(const string& subpath,
  //    const AuthorizerRequest& auth,
  //    Publisher* publisher) = 0;
};

}

#endif // __MEDIA_BASE_PUBLISHER_H__
