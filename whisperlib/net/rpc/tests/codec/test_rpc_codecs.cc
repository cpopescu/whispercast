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

#include <list>

#include "common/base/types.h"
#include "common/base/log.h"
#include "common/base/scoped_ptr.h"
#include "common/io/input_stream.h"
#include "common/io/output_stream.h"
#include "common/io/buffer/io_memory_stream.h"

#include "net/rpc/lib/types/rpc_all_types.h"
#include "net/rpc/lib/codec/rpc_encoder.h"
#include "net/rpc/lib/codec/rpc_decoder.h"
#include "net/rpc/lib/codec/rpc_codec.h"

#include "net/rpc/lib/codec/binary/rpc_binary_encoder.h"
#include "net/rpc/lib/codec/binary/rpc_binary_decoder.h"
#include "net/rpc/lib/codec/binary/rpc_binary_codec.h"

#include "net/rpc/lib/codec/json/rpc_json_encoder.h"
#include "net/rpc/lib/codec/json/rpc_json_decoder.h"
#include "net/rpc/lib/codec/json/rpc_json_codec.h"

#include "auto/types.h"

#define LOG_TEST LOG(-1)

io::MemoryStream* g_pms = NULL;

template <typename T>
void Write(rpc::Encoder& encoder, const T& obj) {
  encoder.Encode(obj);
}

#define TYPENAME(x) #x
template <typename T>
void ReadVerify(rpc::Decoder& decoder, const T& obj) {
  LOG_DEBUG << "ReadVerify decoding " << TYPENAME(T) << " from: "
            << g_pms->DebugString();
  T a;
  rpc::DECODE_RESULT result = decoder.Decode(a);
  CHECK_EQ(result, rpc::DECODE_RESULT_SUCCESS);
  CHECK(a == obj);
}

template <typename T>
void TestPartialDecode(io::MemoryStream& ms,
                       rpc::CODEC_ID codec_id,
                       rpc::Encoder& encoder,
                       rpc::Decoder& decoder,
                       const T& obj) {
  DCHECK(ms.IsEmpty() || ms.DebugString() == " ")
      << "-> [" << ms.DebugString() << "]";
  encoder.Encode(obj);

  io::MemoryStream tmp;
  tmp.AppendStream(&ms);
  CHECK(ms.IsEmpty());

  uint32 size = 0;
  if ( codec_id == rpc::CID_JSON &&
       strcmp(TYPENAME(T), "string") ) {
    // compute size without the ending blanks
    io::MemoryStream in;
    in.AppendStreamNonDestructive(&tmp);
    for ( uint32 i = 0; !in.IsEmpty(); ++i ) {
      char c;
      in.Read(&c, 1);
      if ( c != ' ' ) {
        size = i + 1;
      }
    }
  } else {
    size = tmp.Size();
  }

  T a;
  for ( uint32 i = 0; i < size; ++i ) {
    CHECK(ms.IsEmpty());
    ms.AppendStreamNonDestructive(&tmp, i);
    CHECK_EQ(ms.Size(), i);

    // reset internal state after an incomplete Decode
    decoder.Reset();

    ms.MarkerSet();
    rpc::DECODE_RESULT result = decoder.Decode(a);
    ms.MarkerRestore();

    if ( result != rpc::DECODE_RESULT_NOT_ENOUGH_DATA ) {
      LOG_FATAL << "While decoding partial data. " << std::endl
                << " object: [" << rpc::ToString(obj) << "] type="
                << TYPENAME(T) << std::endl
                << " fully encoded: " << tmp.DebugString() << std::endl
                << " size: " << size << std::endl
                << " partial bytes: #" << i << std::endl
                << " partial data: " << ms.DebugString() << std::endl
                << " result: " << result << std::endl;
    }
    ms.Clear();
  }
  ms.AppendStreamNonDestructive(&tmp);
  decoder.Reset();
  rpc::DECODE_RESULT result = decoder.Decode(a);
  CHECK(result == rpc::DECODE_RESULT_SUCCESS)
      << "Failed to decode "
      << TYPENAME(T) << " from: " << tmp.DebugString();
  CHECK(a == obj) << "Error: "
                  << rpc::ToString(a) << "\n" << rpc::ToString(obj);
  //CHECK(ms.IsEmpty()) << std::endl
  //                    << " tmp = " << tmp.DebugString() << std::endl
  //                    << " ms = [" << ms.DebugString() << "]" << std::endl;
}


template<typename T>
void TestCodec() {
  LOG_INFO << "Begin testing codec: " << TYPENAME(T);

  io::MemoryStream ms;
  g_pms = &ms;

  T codec;
  rpc::Encoder& encoder = *codec.CreateEncoder(ms);
  rpc::Decoder& decoder = *codec.CreateDecoder(ms);

  scoped_ptr<rpc::Encoder> auto_del_encoder(&encoder);
  scoped_ptr<rpc::Decoder> auto_del_decoder(&decoder);

  vector<rpc::Void> arrVoid0(0);
  vector<bool> arrBool0(0);
  vector<bool> arrBool3(3);
  arrBool3[0] = true;
  arrBool3[1] = false;
  arrBool3[2] = true;
  vector<string> arrString0(0);
  vector<string> arrString3(3);
  arrString3[0] = "a";
  arrString3[1] = "";
  arrString3[2] = "b";
  vector<double> arrFloat0(0);
  vector<double> arrFloat3(3);
  arrFloat3[0] = 1.71;
  arrFloat3[1] = 2.72;
  arrFloat3[2] = 3.73;
  vector<int32> arrInt0(0);
  vector<int32> arrInt3(3);
  arrInt3[0] = 3;
  arrInt3[1] = 2;
  arrInt3[2] = 0x7fffffff;
  map<bool, bool> mapBoolBool0;
  map<bool, bool> mapBoolBool3;
  mapBoolBool3.insert(make_pair(true, true));
  mapBoolBool3.insert(make_pair(false, false));
  mapBoolBool3.insert(make_pair(false, true));
  map<int32, rpc::Void> mapIntVoid0;
  map<int32, double> mapIntFloat3;
  mapIntFloat3.insert(make_pair(1, 1.1f));
  mapIntFloat3.insert(make_pair(2, 1.2f));
  mapIntFloat3.insert(make_pair(3, 1.3f));
  map<string, string> mapStringString0;
  map<string, string> mapStringString3;
  mapStringString3.insert(make_pair(string("a"), string("b")));
  mapStringString3.insert(make_pair(string(""), string("")));
  mapStringString3.insert(make_pair(string(""), string("qwerty")));
  Circle circle;
  circle.radius_ = double(1.7f);
  Rectangle rect;
  rect.width_ = int32(3);
  rect.height_ = int32(5);
  Car car1;
  car1.name_ = string("my first car");
  car1.hp_ = int32(33);
  car1.speed_ = double(0.71828182f);
  car1.persons_.resize(0);
  car1.performance_.resize(0);
  Car car2;
  car2.name_ = string("my second car");
  car2.hp_ = int32(22);
  car2.speed_ = double(1.91f);
  car2.persons_.push_back("aabbcc");
  car2.performance_.resize(1);
  car2.performance_[0].resize(1);
  car2.performance_[0][0] = 4.3f;
  Car car3;
  car3.name_ = string("my third car");
  car3.hp_ = int32(77);
  car3.speed_ = double(2.71828182f);
  car3.persons_.push_back("abc");
  car3.persons_.push_back("bca");
  car3.persons_.push_back("cab");
  vector<double> car3perf(5);
  car3perf[0] = 0;
  car3perf[1] = 1;
  car3perf[2] = 2;
  car3perf[3] = 3;
  car3perf[4] = 4;

  car3.performance_.push_back(car3perf);
  City city1;
  city1.name_ = "my first city";
  city1.ids_.resize(0);
  city1.citizens_.ref();  // to set citizens to an empty map
  City city2;
  city2.name_ = string("my second city");
  city2.ids_.resize(1);
  city2.ids_[0] = 3;
  city2.citizens_.insert(make_pair(int32(3), string("gigel")));
  City city3;
  city3.name_ = string("my third city");
  city3.ids_.resize(2);
  city3.ids_[0] = 7;
  city3.ids_[1] = 9;
  city3.citizens_.insert(make_pair(int32(7), string("gigel")));
  city3.citizens_.insert(make_pair(int32(9), string("mitica")));

  LOG_INFO << "################### SINGLE TESTS ####################";
  {
#define SINGLE_TEST(obj) do {                                   \
      LOG_INFO << " Testing: [" << rpc::ToString(obj) << "]";      \
      Write(encoder, obj);                                      \
      LOG_INFO << "To Decode: [" << ms.DebugString() << "]";    \
      ReadVerify(decoder, obj);                                 \
      LOG_INFO << "OK SINGLE_TEST " #obj;                       \
    } while(false)
    // CHECK_EQ(ms.Size(), 1) << "LEFT: [" << ms.DebugString() << "]";
    SINGLE_TEST(rpc::Void());
    SINGLE_TEST(bool(true));
    SINGLE_TEST(int32(3));
    SINGLE_TEST(int32(-3));
    SINGLE_TEST(uint32(12));
    SINGLE_TEST(int64(17));
    SINGLE_TEST(int64(-17));
    SINGLE_TEST(uint64(19));
    SINGLE_TEST(double(4.3333333333333333333333333333333333333333333f));
    SINGLE_TEST(double(4.333333333333333333333333333333333333333333333333333333333333333333333333));
    SINGLE_TEST(string(""));
    SINGLE_TEST(string("abcdef"));
    SINGLE_TEST(arrVoid0);
    SINGLE_TEST(arrBool0);
    SINGLE_TEST(arrBool3);
    SINGLE_TEST(arrString0);
    SINGLE_TEST(arrString3);
    SINGLE_TEST(arrFloat0);
    SINGLE_TEST(arrFloat3);
    SINGLE_TEST(arrInt0);
    SINGLE_TEST(arrInt3);
    SINGLE_TEST(mapBoolBool0);
    SINGLE_TEST(mapBoolBool3);
    SINGLE_TEST(mapIntVoid0);
    SINGLE_TEST(mapIntFloat3);
    SINGLE_TEST(mapStringString3);
    SINGLE_TEST(circle);
    SINGLE_TEST(rect);
    {
      Write(encoder, car1);
      Car a;
      const rpc::DECODE_RESULT result = decoder.Decode(a);
      CHECK_EQ(result, rpc::DECODE_RESULT_SUCCESS);
      CHECK(car1.name_ == a.name_);
      CHECK(car1.hp_ ==  a.hp_);
      CHECK(car1.speed_ == a.speed_);
      CHECK(car1.persons_ == a.persons_);
      CHECK(car1.performance_ == a.performance_);
    }
    SINGLE_TEST(car1);
    SINGLE_TEST(car2);
    SINGLE_TEST(car3);
    SINGLE_TEST(city1);
    SINGLE_TEST(city2);
    SINGLE_TEST(city3);
  }
  CHECK(ms.IsEmpty());

  LOG_INFO << "################### DOUBLE TESTS ####################";
  {
#define DOUBLE_TEST_EXCLUSIVE(a, b) {                           \
      Write(encoder, a);                                        \
      Write(encoder, b);                                        \
      ReadVerify(decoder, a);                                   \
      ReadVerify(decoder, b);                                   \
      LOG_INFO << "OK DOUBLE_TEST_EXCLUSIVE " #a " , " #b;      \
    }
#define DOUBLE_TEST(a, b) {                             \
      SINGLE_TEST(a);                                   \
      SINGLE_TEST(b);                                   \
      DOUBLE_TEST_EXCLUSIVE(a, b);                      \
      DOUBLE_TEST_EXCLUSIVE(b, a);                      \
      LOG_INFO << "OK DOUBLE_TEST " #a " , " #b;        \
    }
    DOUBLE_TEST_EXCLUSIVE(rpc::Void(), rpc::Void());
    DOUBLE_TEST(rpc::Void(), rpc::Void());
    DOUBLE_TEST(rpc::Void(), int32(5));
    DOUBLE_TEST(rpc::Void(), string("abcdef"));
    DOUBLE_TEST(int32(7), string(""));
    DOUBLE_TEST(int32(7), string("qwerty"));
    DOUBLE_TEST(double(7.6f), double(8.7));
    DOUBLE_TEST(string("a"), string("b"));
    DOUBLE_TEST(string(""), string(""));
    DOUBLE_TEST(string(""), rpc::Void());
    DOUBLE_TEST(string("abc"), rpc::Void());
    DOUBLE_TEST(arrString3, mapStringString3);
    DOUBLE_TEST(arrString0, arrFloat3);
    DOUBLE_TEST(arrInt3, arrFloat0);
    DOUBLE_TEST(arrFloat3, arrString3);
    DOUBLE_TEST(arrVoid0, mapIntVoid0);
    DOUBLE_TEST(arrBool3, mapIntFloat3);
    DOUBLE_TEST(mapIntFloat3, mapIntFloat3);
    DOUBLE_TEST(circle, mapIntFloat3);
    DOUBLE_TEST(city2, mapIntFloat3);
    DOUBLE_TEST(city1, car1);
    DOUBLE_TEST(city2, car2);
    DOUBLE_TEST(city3, car3);
    DOUBLE_TEST(rect, circle);
    DOUBLE_TEST(rect, car3);
    DOUBLE_TEST(car3, arrFloat3);
  }
  CHECK(ms.IsEmpty());

  LOG_INFO << "################### TRIPLE TESTS ####################";
  {
#define TRIPLE_TEST_EXCLUSIVE(a, b, c) {                                \
      Write(encoder, a);                                                \
      Write(encoder, b);                                                \
      Write(encoder, c);                                                \
      ReadVerify(decoder, a);                                           \
      ReadVerify(decoder, b);                                           \
      ReadVerify(decoder, c);                                           \
      LOG_INFO << "OK TRIPLE_TEST_EXCLUSIVE " #a " , " #b " , " #c;     \
    }
#define TRIPLE_TEST(a, b, c) {                                  \
      SINGLE_TEST(a);                                           \
      SINGLE_TEST(b);                                           \
      SINGLE_TEST(c);                                           \
      DOUBLE_TEST_EXCLUSIVE(a, b);                              \
      DOUBLE_TEST_EXCLUSIVE(b, c);                              \
      DOUBLE_TEST_EXCLUSIVE(a, c);                              \
      TRIPLE_TEST_EXCLUSIVE(a, b, c);                           \
      TRIPLE_TEST_EXCLUSIVE(a, c, b);                           \
      TRIPLE_TEST_EXCLUSIVE(b, a, c);                           \
      TRIPLE_TEST_EXCLUSIVE(b, c, a);                           \
      TRIPLE_TEST_EXCLUSIVE(c, a, b);                           \
      TRIPLE_TEST_EXCLUSIVE(c, b, a);                           \
      LOG_INFO << "OK TRIPLE_TEST " #a " , " #b " , " #c;       \
    }
    TRIPLE_TEST(rpc::Void(), int32(4), bool(true));
    TRIPLE_TEST(int32(), double(4.1f), double(1.5));
    TRIPLE_TEST(string(""), rpc::Void(), rpc::Void());
    TRIPLE_TEST(string("abc"), rpc::Void(), double(1.2f));
    TRIPLE_TEST(bool(false), string(""), int32());
    TRIPLE_TEST(bool(true), string("qwe"), double(1.7));
    TRIPLE_TEST(string("asd"), string(""), int32());
    TRIPLE_TEST(string("asd"), string("dsa"), int32());
    TRIPLE_TEST(string(""), string("dsa"), string(""));
    TRIPLE_TEST(string("a"), string("b"), string("c"));
    TRIPLE_TEST(arrString3, mapStringString3, rpc::Void());
    TRIPLE_TEST(arrString3, mapStringString3, string("xyz"));
    TRIPLE_TEST(bool(false), arrString3, mapStringString3);
    TRIPLE_TEST(mapStringString3, rpc::Void(), arrString3);
    TRIPLE_TEST(mapStringString3, double(3.4), arrString3);
    TRIPLE_TEST(mapStringString3, int32(3), mapStringString3);
    TRIPLE_TEST(mapStringString3, string("a"), string("b"));
    TRIPLE_TEST(mapStringString3, arrString3, arrString3);
    TRIPLE_TEST(arrString3, mapStringString3, arrString3);
    TRIPLE_TEST(mapStringString3, mapStringString3, mapStringString3);
    TRIPLE_TEST(arrString3, arrString3, arrString3);
    TRIPLE_TEST(arrString3, mapStringString3, string("xyz"));
    TRIPLE_TEST(bool(true), arrInt3, mapStringString3);
    TRIPLE_TEST(mapStringString0, rpc::Void(), arrFloat0);
    TRIPLE_TEST(mapIntFloat3, double(3.4), arrInt3);
    TRIPLE_TEST(mapStringString3, car3, circle);
    TRIPLE_TEST(city3, string("a"), string("b"));
    TRIPLE_TEST(city3, car3, rect);
    TRIPLE_TEST(arrString3, city2, car2);
    TRIPLE_TEST(car3, car3, car3);
    TRIPLE_TEST(city2, city2, city2);
    TRIPLE_TEST(arrBool0, arrBool3, rpc::Void());
    TRIPLE_TEST(mapIntFloat3, mapIntFloat3, string("xyz"));
    TRIPLE_TEST(bool(false), city2, circle);
    TRIPLE_TEST(car2, rpc::Void(), rect);
    TRIPLE_TEST(arrFloat3, double(3.4), car3);
    TRIPLE_TEST(arrFloat0, int32(9), mapStringString3);
    TRIPLE_TEST(arrString3, string("a"), string("b"));
    TRIPLE_TEST(arrString0, arrInt0, arrFloat0);
    TRIPLE_TEST(rect, circle, arrInt3);
    TRIPLE_TEST(car1, car3, car2);
    TRIPLE_TEST(city1, city2, city3);
  }
  CHECK(ms.IsEmpty());

  {
#define ENCODE_PACKET(p) {                      \
      encoder.EncodePacket(p);                  \
    }
#define EXPECT_PACKET(expected) {                                       \
      LOG_DEBUG << "EXPECT_PACKET decoding rpc::Message with codec "    \
                << TYPENAME(T) << " from: " << ms.DebugString();        \
      rpc::Message found;                                               \
      const rpc::DECODE_RESULT result = decoder.DecodePacket(found);    \
      CHECK_EQ(result, rpc::DECODE_RESULT_SUCCESS);                     \
      CHECK_EQ(expected, found);                                        \
      LOG_INFO << "OK EXPECT_PACKET " #expected ;                       \
    }
#define ENCODE_VALUE(v) {                       \
      encoder.Encode(v);                        \
    }
#define EXPECT_VALUE(expected) {                        \
      ReadVerify(decoder, expected);                    \
      LOG_INFO << "OK EXPECT_VALUE " #expected ;        \
    }
    rpc::Message msg0;
    msg0.header_.msgType_ = rpc::RPC_CALL;
    msg0.header_.xid_ = 0xffffffff;
    msg0.cbody_.service_ = "my service";
    msg0.cbody_.method_ = "my method";
    msg0.cbody_.params_.Write("abcdef", 6);
    rpc::Message msg1;
    msg1.header_.msgType_ = rpc::RPC_CALL;
    msg1.header_.xid_ = 0x12345678;
    msg1.cbody_.service_ = "service 2";
    msg1.cbody_.method_ = "method 2";
    msg1.cbody_.params_.Write("{}");
    rpc::Message msg2;
    msg2.header_.msgType_ = rpc::RPC_REPLY;
    msg2.header_.xid_ = 0xffffffff;
    msg2.rbody_.replyStatus_ = rpc::RPC_SERVICE_UNAVAIL;
    msg2.rbody_.result_.Write("1234567890");
    rpc::Message msg3;
    msg3.header_.msgType_ = rpc::RPC_REPLY;
    msg3.header_.xid_ = 0x11111111;
    msg3.rbody_.replyStatus_ = rpc::RPC_SUCCESS;
    msg3.rbody_.result_.Write("0-1-2-3-4-5-6-7-8-9");

    ENCODE_PACKET(msg0);
    EXPECT_PACKET(msg0);
    CHECK(ms.IsEmpty());

    ENCODE_PACKET(msg0);
    ENCODE_PACKET(msg1);
    EXPECT_PACKET(msg0);
    EXPECT_PACKET(msg1);
    CHECK(ms.IsEmpty());

    ENCODE_PACKET(msg0);
    ENCODE_PACKET(msg1);
    ENCODE_PACKET(msg2);
    EXPECT_PACKET(msg0);
    EXPECT_PACKET(msg1);
    EXPECT_PACKET(msg2);
    CHECK(ms.IsEmpty());

    ENCODE_PACKET(msg0);
    ENCODE_PACKET(msg1);
    ENCODE_PACKET(msg2);
    ENCODE_PACKET(msg3);
    EXPECT_PACKET(msg0);
    EXPECT_PACKET(msg1);
    EXPECT_PACKET(msg2);
    EXPECT_PACKET(msg3);
    CHECK(ms.IsEmpty());

    ENCODE_PACKET(msg0);
    ENCODE_VALUE(circle);
    ENCODE_PACKET(msg1);
    ENCODE_VALUE(car3);
    ENCODE_PACKET(msg2);
    ENCODE_VALUE(city3);
    ENCODE_PACKET(msg3);
    ENCODE_VALUE(rect);
    EXPECT_PACKET(msg0);
    EXPECT_VALUE(circle);
    EXPECT_PACKET(msg1);
    EXPECT_VALUE(car3);
    EXPECT_PACKET(msg2);
    EXPECT_VALUE(city3);
    EXPECT_PACKET(msg3);
    EXPECT_VALUE(rect);
    CHECK(ms.IsEmpty());
  }

  LOG_INFO << "################### Partial message TESTS ####################";
  {
#define PARTIAL_TEST(v) {                                               \
      TestPartialDecode(ms, codec.GetCodecID(), encoder, decoder, v);   \
      LOG_INFO << "OK Partial test " #v ;                               \
    }
    // CHECK(ms.IsEmpty());
    // CHECK(ms.IsEmpty());
    PARTIAL_TEST(rpc::Void());
    PARTIAL_TEST(bool(true));
    PARTIAL_TEST(int32(3));
    PARTIAL_TEST(int32(-3));
    PARTIAL_TEST(uint32(12));
    PARTIAL_TEST(int64(17));
    PARTIAL_TEST(int64(-17));
    PARTIAL_TEST(uint64(19));
    PARTIAL_TEST(double(4.3333333333333333333333333333333333333333333f));
    PARTIAL_TEST(double(4.333333333333333333333333333333333333333333333333333333333333333333333333));
    PARTIAL_TEST(string(""));
    PARTIAL_TEST(string("abcdef"));
    PARTIAL_TEST(arrVoid0);
    PARTIAL_TEST(arrBool0);
    PARTIAL_TEST(arrBool3);
    PARTIAL_TEST(arrString0);
    PARTIAL_TEST(arrString3);
    PARTIAL_TEST(arrFloat0);
    PARTIAL_TEST(arrFloat3);
    PARTIAL_TEST(arrInt0);
    PARTIAL_TEST(arrInt3);
    PARTIAL_TEST(mapBoolBool0);
    PARTIAL_TEST(mapBoolBool3);
    PARTIAL_TEST(mapIntVoid0);
    PARTIAL_TEST(mapIntFloat3);
    PARTIAL_TEST(mapStringString3);
    PARTIAL_TEST(circle);
    PARTIAL_TEST(rect);
    PARTIAL_TEST(car1);
    PARTIAL_TEST(car2);
    PARTIAL_TEST(car3);
    PARTIAL_TEST(city1);
    PARTIAL_TEST(city2);
    PARTIAL_TEST(city3);
    {
      rpc::Message msg;
      msg.header_.msgType_ = rpc::RPC_CALL;
      msg.header_.xid_ = 0xffffffff;
      msg.cbody_.service_ = "my service";
      msg.cbody_.method_ = "my method";
      msg.cbody_.params_.Write("abcdef", 6);
      PARTIAL_TEST(msg);
    }
    {
      rpc::Message msg;
      msg.header_.msgType_ = rpc::RPC_REPLY;
      msg.header_.xid_ = 0x12345678;
      msg.rbody_.replyStatus_ = rpc::RPC_PROC_UNAVAIL;
      msg.rbody_.result_.Write("fedcba");
      PARTIAL_TEST(msg);
    }
    CHECK(ms.IsEmpty());
  }

  LOG_INFO << "Pass codec: " << TYPENAME(T);
}

template <typename T>
void ShowEncoding(rpc::Codec& codec, bool show_text, T value) {
  io::MemoryStream ms;
  scoped_ptr<rpc::Encoder> encoder(codec.CreateEncoder(ms));
  encoder->Encode(value);
  if ( show_text ) {
    string text;
    ms.ReadString(&text);
    LOG_TEST << TYPENAME(T) << " value: " << rpc::ToString(value)
             << " was encoded to: \"" << text << "\"";
  } else {
    LOG_TEST << TYPENAME(T) << " value: " << rpc::ToString(value) << " was encoded to: "
             << ms.DebugString();
  }
}

void ShowPacketEncoding(rpc::Codec& codec, bool show_text, rpc::Message& msg) {
  io::MemoryStream ms;
  scoped_ptr<rpc::Encoder> encoder(codec.CreateEncoder(ms));
  encoder->EncodePacket(msg);
  if ( show_text ) {
    string text;
    ms.ReadString(&text);
    LOG_TEST << msg << " was encoded to: \"" << text << "\"";
  } else {
    LOG_TEST << msg << " was encoded to: " << ms.DebugString();
  }
}

template <typename CODEC>
void ShowEncoding(bool show_text) {
  CODEC _codec;
  rpc::Codec & codec = _codec;

  ShowEncoding(codec, show_text, rpc::Void());
  ShowEncoding(codec, show_text, bool(true));
  ShowEncoding(codec, show_text, bool(false));
  ShowEncoding(codec, show_text, int32(7));
  ShowEncoding(codec, show_text, int32(-3));
  ShowEncoding(codec, show_text, double(2.6f));
  ShowEncoding(codec, show_text, string(""));
  ShowEncoding(codec, show_text, string("abcdef"));
  vector<bool> arrBool0(0);
  vector<bool> arrBool3(3);
  arrBool3[0] = false;
  arrBool3[1] = true;
  arrBool3[2] = false;
  vector<int32> arrInt0(0);
  vector<int32> arrInt3(3);
  arrInt3[0] = false;
  arrInt3[1] = true;
  arrInt3[2] = false;
  vector<double> arrFloat0(0);
  vector<double> arrFloat3(3);
  arrFloat3[0] = false;
  arrFloat3[1] = true;
  arrFloat3[2] = false;
  vector<string> arrString0(0);
  vector<string> arrString3(3);
  arrString3[0] = "";
  arrString3[1] = "a";
  arrString3[2] = "x y z";
  ShowEncoding(codec, show_text, arrBool0);
  ShowEncoding(codec, show_text, arrBool3);
  ShowEncoding(codec, show_text, arrInt0);
  ShowEncoding(codec, show_text, arrInt3);
  ShowEncoding(codec, show_text, arrFloat0);
  ShowEncoding(codec, show_text, arrFloat3);
  ShowEncoding(codec, show_text, arrString0);
  ShowEncoding(codec, show_text, arrString3);

  {
    vector<vector<bool> > a;
    ShowEncoding(codec, show_text, a);
    a.resize(1);
    ShowEncoding(codec, show_text, a);
    a.resize(2);
    a[0].resize(1);
    a[0][0] = true;
    a[1].resize(2);
    a[1][0] = false;
    a[1][1] = true;
    ShowEncoding(codec, show_text, a);
  }
  {
    vector<vector<vector<string> > > a;
    ShowEncoding(codec, show_text, a);
    a.resize(1);
    a[0].resize(1);
    a[0][0].resize(1);
    a[0][0][0] = "a b c";
    ShowEncoding(codec, show_text, a);
    a[0][0].resize(2);
    a[0][0][1] = "c b a";
    ShowEncoding(codec, show_text, a);
  }
  {
    map<int32, string> m;
    ShowEncoding(codec, show_text, m);
    m.insert(make_pair(int32(1), string("a b c")));
    ShowEncoding(codec, show_text, m);
    m.insert(make_pair(int32(3), string("123")));
    ShowEncoding(codec, show_text, m);
    m.insert(make_pair(int32(5), string("x_y_z")));
    ShowEncoding(codec, show_text, m);
  }
  {
    map<string, vector<int32> > m;
    ShowEncoding(codec, show_text, m);
    vector<int32> a(0);
    m.insert(make_pair(string("first"), a));
    ShowEncoding(codec, show_text, m);
    a.resize(1);
    a[0] = 1;
    m.insert(make_pair(string("second"), a));
    ShowEncoding(codec, show_text, m);
    a.resize(2);
    a[0] = 0;
    a[1] = 1;
    m.insert(make_pair(string("third"), a));
    ShowEncoding(codec, show_text, m);
  }
  // custom types
  {
    Circle circle;
    circle.radius_ = 1.7f;
    ShowEncoding(codec, show_text, circle);
  }
  {
    Rectangle rect;
    rect.width_ = 3;
    rect.height_ = 5;
    ShowEncoding(codec, show_text, rect);
  }
  {
    Car car;
    car.name_ = string("my car");
    car.hp_ = int32(77);
    car.speed_ = double(2.71828182f);
    car.persons_.push_back("abc");
    car.persons_.push_back("bca");
    car.persons_.push_back("cab");
    vector<double> cperf(5);
    cperf[0] = 0;
    cperf[1] = 1;
    cperf[2] = 2;
    cperf[3] = 3;
    cperf[4] = 4;
    // cpopescu
    car.performance_.push_back(cperf);
    ShowEncoding(codec, show_text, car);
  }
  {
    City city;
    city.name_ = string("my third city");
    city.ids_.resize(2);
    city.ids_[0] = 7;
    city.ids_[1] = 9;
    city.citizens_.insert(make_pair(int32(7), string("gigel")));
    city.citizens_.insert(make_pair(int32(9), string("mitica")));
    ShowEncoding(codec, show_text, city);
  }
  {
    rpc::Message msg;
    msg.header_.msgType_ = rpc::RPC_CALL;
    msg.header_.xid_ = 0xffffffff;
    msg.cbody_.service_ = "my service";
    msg.cbody_.method_ = "my method";
    msg.cbody_.params_.Write("abcdef", 6);
    ShowPacketEncoding(codec, show_text, msg);
  }
  {
    rpc::Message msg;
    msg.header_.msgType_ = rpc::RPC_REPLY;
    msg.header_.xid_ = 0x12345678;
    msg.rbody_.replyStatus_ = rpc::RPC_PROC_UNAVAIL;
    msg.rbody_.result_.Write("fedcba");
    ShowPacketEncoding(codec, show_text, msg);
  }
}

int main(int argc, char** argv) {
  common::Init(argc, argv);

  TestCodec<rpc::JsonCodec>();
  TestCodec<rpc::BinaryCodec>();

  ShowEncoding<rpc::JsonCodec>(true);

  LOG_INFO << "Pass";
  common::Exit(0);
  return 0;
}
