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

#ifndef __NET_RPC_LIB_CODEC_BINARY_RPC_BINARY_ENCODER_H__
#define __NET_RPC_LIB_CODEC_BINARY_RPC_BINARY_ENCODER_H__

#include <whisperlib/net/rpc/lib/codec/rpc_encoder.h>
#include <whisperlib/net/rpc/lib/codec/binary/rpc_binary_common.h>

namespace rpc {

class BinaryEncoder : public rpc::Encoder {
 public:
  explicit BinaryEncoder(io::MemoryStream& out)
      : rpc::Encoder(out) {
  }
  virtual ~BinaryEncoder() {
  }

  //////////////////////////////////////////////////////////////////////
  //
  //                   rpc::Encoder interface methods
  //
  void EncodeStructStart(uint32 nAttribs) {  Encode(nAttribs); }
  void EncodeStructContinue()             {  }
  void EncodeStructEnd()                  {  }
  void EncodeStructAttribStart()          {  }
  void EncodeStructAttribMiddle()         {  }
  void EncodeStructAttribEnd()            {  }
  void EncodeArrayStart(uint32 nElements) {  Encode(nElements); }
  void EncodeArrayContinue()              {  }
  void EncodeArrayEnd()                   {  }
  void EncodeArrayElementStart()          {  }
  void EncodeArrayElementEnd()            {  }
  void EncodeMapStart(uint32 nPairs)      {   Encode(nPairs);  }
  void EncodeMapContinue()                {  }
  void EncodeMapEnd()                     {  }
  void EncodeMapPairStart()               {  }
  void EncodeMapPairMiddle()              {  }
  void EncodeMapPairEnd()                 {  }

 protected:
  void EncodeBody(const rpc::Void& obj)    {
    UNUSED_ALWAYS(obj);
    io::NumStreamer::WriteByte(out_, 0xff);
  }
  void EncodeBody(const bool& obj)    {
    io::NumStreamer::WriteByte(out_, obj == true ? 1 : 0);
  }
 private:
  // A simple helper function..
  template <typename T>
  void EncodeNumeric(const T& val) {
    io::NumStreamer::WriteNumber<T>(
        out_, val, rpc::kBinaryByteOrder);
  }
 protected:
  void EncodeBody(const int32& obj)  { EncodeNumeric(obj); }
  void EncodeBody(const uint32& obj) { EncodeNumeric(obj); }
  void EncodeBody(const int64& obj)  { EncodeNumeric(obj); }
  void EncodeBody(const uint64& obj) { EncodeNumeric(obj); }
  void EncodeBody(const double& obj) { EncodeNumeric(obj); }

  void EncodeBody(const string& obj)  {
    io::NumStreamer::WriteInt32(out_, obj.size(), rpc::kBinaryByteOrder);
    out_->Write(obj.data(), obj.size());
  }
  void EncodeBody(const char* obj)  {
    const int32 len = strlen(obj);
    io::NumStreamer::WriteInt32(out_, len, rpc::kBinaryByteOrder);
    out_->Write(obj, len);
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(BinaryEncoder);
};
}
#endif   // __NET_RPC_LIB_CODEC_BINARY_RPC_BINARY_ENCODER_H__
