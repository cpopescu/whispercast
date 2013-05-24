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
// Author: Catalin Popescu

//
// Simple test for all rtmp object classes
//
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/io/buffer/memory_stream.h>

#include "rtmp/objects/rtmp_objects.h"
#include "rtmp/objects/amf/amf0_util.h"
#include "rtmp/objects/amf/amf_util.h"

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  vector <rtmp::CObject*> initial;

  initial.push_back(new rtmp::CNumber(1.23));
  initial.push_back(new rtmp::CNull());
  initial.push_back(new rtmp::CBoolean(true));
  initial.push_back(new rtmp::CString("abcdef"));
  initial.push_back(new rtmp::CDate(4222, 12300));

  rtmp::CMixedMap* im1 = new rtmp::CMixedMap();
  im1->Set("12", new rtmp::CString("hrksah"));
  im1->Set("K3", new rtmp::CString("fsd;juhh"));
  im1->Set("true_value", new rtmp::CBoolean(true));
  im1->Set("gigi2334844", new rtmp::CDate(4222, 12300));
  im1->Set("ff-10", new rtmp::CNumber(328382.33));
  initial.push_back(im1);

  rtmp::CMixedMap* im2 = new rtmp::CMixedMap();
  im1->Set("true_key", new rtmp::CBoolean(true));
  im1->Set("string1_key", new rtmp::CString("string1_value"));
  im1->Set("string2_key", new rtmp::CString("string2_value"));
  im1->Set("num1_key", new rtmp::CNumber(123.456));
  im1->Set("num2_key", new rtmp::CNumber(0));
  im1->Set("another1_map", im1->Clone());
  initial.push_back(im2);

  initial.push_back(new rtmp::CBoolean(true));
  initial.push_back(new rtmp::CString("1234 5678 abcdef"));

  rtmp::CArray* a1 = new rtmp::CArray();
  a1->mutable_data().push_back(new rtmp::CNumber(13245));
  a1->mutable_data().push_back(new rtmp::CBoolean(true));
  a1->mutable_data().push_back(new rtmp::CNumber(2397842));
  a1->mutable_data().push_back(new rtmp::CDate(239784223, 48399));
  a1->mutable_data().push_back(new rtmp::CNumber(1.2345));
  a1->mutable_data().push_back(new rtmp::CString("yuyeks sjhf aklfh "));
  initial.push_back(a1);

  initial.push_back(new rtmp::CNumber(213474));
  initial.push_back(new rtmp::CBoolean(false));
  initial.push_back(new rtmp::CString("agdafsf6dars aaoljf"));

  rtmp::CStringMap* sm1 = new rtmp::CStringMap();
  sm1->Set("askjhyr mkja", new rtmp::CNumber(3827.3290));
  sm1->Set("salf", new rtmp::CString("198274nhsd fldsj"));
  sm1->Set("posopo", new rtmp::CDate(12873242, 3600));
  sm1->Set("askjhyr mkja", new rtmp::CNumber(3827.3290));
  initial.push_back(sm1);

  initial.push_back(new rtmp::CRecordSet());
  initial.push_back(new rtmp::CRecordSetPage());


  io::MemoryStream ms;

  rtmp::CObject* tmp;
  CHECK_EQ(rtmp::Amf0Util::ReadNextObject(&ms, &tmp),
           rtmp::AmfUtil::READ_NO_DATA);
  ms.Write("shjasdlajflkj");
  CHECK_EQ(rtmp::Amf0Util::ReadNextObject(&ms, &tmp),
           rtmp::AmfUtil::READ_CORRUPTED_DATA);
  ms.Clear();

  for ( int i = 0; i < initial.size(); ++i ) {
    rtmp::CObject* p = initial[i];
    LOG_INFO << "Writing: " << *p;
    int32 start_size = ms.Size();
    p->Encode(&ms, rtmp::AmfUtil::AMF0_VERSION);
    int32 encoding_size = ms.Size() - start_size;
    if ( encoding_size != p->EncodingSize(rtmp::AmfUtil::AMF0_VERSION) ) {
      LOG_FATAL << "Wrong size, obj.size: "
                << p->EncodingSize(rtmp::AmfUtil::AMF0_VERSION)
                << ", ms.size: " << encoding_size
                << ", for: " << p->ToString();
    }
  }
  for ( int i = 0; i < initial.size(); ++i ) {
    const rtmp::AmfUtil::ReadStatus err =
      rtmp::Amf0Util::ReadNextObject(&ms, &tmp);
    CHECK(err == rtmp::AmfUtil::READ_OK)
      << " Error: " << rtmp::AmfUtil::ReadStatusName(err);
    LOG_INFO << "Found: " << *tmp;
    CHECK(tmp->Equals(*initial[i])) << "\nFound: \n"
                                   << *tmp << "\nExpected:\n" << *initial[i];
    delete tmp;
    delete initial[i];
  }
  CHECK(ms.IsEmpty());

  LOG_INFO << "PASS";

  common::Exit(0);
}
