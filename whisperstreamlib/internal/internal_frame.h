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
// Author: Mihai Ianculescu

#ifndef __MEDIA_INTERNAL_INTERNAL_FRAME_H__
#define __MEDIA_INTERNAL_INTERNAL_FRAME_H__

#include <string>
#include <whisperlib/common/base/types.h>
#include <whisperstreamlib/base/consts.h>
#include <whisperstreamlib/base/tag.h>

namespace io {
class MemoryStream;
}

namespace streaming {

class InternalFrameTag : public streaming::Tag  {
 public:
  static const Type kType;
  InternalFrameTag(uint32 attributes,
                   uint32 flavour_mask,
                   const string& mime_type,
                   const string& data)
      : Tag(kType, attributes, flavour_mask),
        mime_type_(mime_type),
        header_(),
        data_(data) {
  }
  InternalFrameTag(const InternalFrameTag& other)
    : Tag(other),
      mime_type_(other.mime_type_),
      header_(other.header_),
      data_(other.data_) {
  }
  virtual ~InternalFrameTag() {
  }

  // Reads frame data from the given input stream. Call it multiple times
  // until READ_OK is returned. On READ_OK it advances the stream pointer,
  // else will leave it as it has received it.
  // If synchronize is set, we synchronize w/ the first frame
  streaming::TagReadStatus Read(io::MemoryStream* in);

  const char* ContentType() const {
    return mime_type_.c_str();
  }
  virtual int64 timestamp_ms() const { return header_.timestamp_; }
  virtual int64 duration_ms() const { return 0; }
  virtual uint32 size() const { return data_.size(); }
  virtual Tag* Clone(int64 timestamp_ms) const {
    return new InternalFrameTag(*this);
  }
  virtual string ToStringBody() const;

  const string& mime_type() const { return mime_type_; }
  const streaming::ProtocolFrameHeader& header() const { return header_; }
  const string& data() const { return data_; }

 protected:
  //friend class InternalTagSplitter;

  // The actual mime type of the tag.
  string mime_type_;

  // Data extracted from the internal frame header
  streaming::ProtocolFrameHeader header_;
  // The raw data of the frame
  string data_;

  // The bytes left to be read
  enum State {
    kHeaderFlags,
    kHeaderLength,
    kHeaderTimeStamp,
    kHeaderMimeType,
    kHeaderMimeTypeData,
    kData
  };
};

//////////////////////////////////////////////////////////////////////

class InternalTagSerializer : public streaming::TagSerializer {
 public:
  InternalTagSerializer() : streaming::TagSerializer() { }
  virtual ~InternalTagSerializer() {}
  virtual void Initialize(io::MemoryStream* out) {}
  virtual void Finalize(io::MemoryStream* out) {}
 protected:
  virtual bool SerializeInternal(const Tag* tag, int64 base_timestamp_ms,
                                 io::MemoryStream* out) {
    if ( tag->type() != Tag::TYPE_INTERNAL ) {
      return false;
    }
    out->Write(reinterpret_cast<const InternalFrameTag*>(tag)->data());
    return true;
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(InternalTagSerializer);
};

}

#endif  // __MEDIA_INTERNAL_INTERNAL_FRAME_H__
