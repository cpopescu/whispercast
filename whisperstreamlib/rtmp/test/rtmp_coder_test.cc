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


#include <deque>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperstreamlib/rtmp/objects/amf/amf0_util.h>
#include <whisperstreamlib/rtmp/objects/amf/amf_util.h>
#include <whisperstreamlib/rtmp/events/rtmp_event.h>
#include <whisperstreamlib/rtmp/rtmp_coder.h>

//////////////////////////////////////////////////////////////////////

DEFINE_int32(num_ops,
             10000,
             " Number of operations in the test");
DEFINE_int32(fixed_event,
             -1,
             "Write just one type of event..");

//////////////////////////////////////////////////////////////////////

template<class C>
C* PrepareBulkData(const rtmp::Header& header, int size) {
  C* ret = new C(header);
  for ( int i = 0; i < size; i++ ) {
    const int x = random();
    ret->mutable_data()->Write(&x, sizeof(x));
  }
  return ret;
}

uint32 rnd() { return random() % 0xffffffff; }

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  vector <scoped_ref<rtmp::Event> > initial;

  initial.push_back(new rtmp::EventChunkSize(rtmp::Header(2, 1,
      rtmp::EVENT_CHUNK_SIZE, rnd(), false), 256));
  initial.push_back(new rtmp::EventBytesRead(rtmp::Header(2, 1,
      rtmp::EVENT_BYTES_READ, rnd(), false), 123));
  initial.push_back(new rtmp::EventServerBW(rtmp::Header(2, 1,
      rtmp::EVENT_SERVER_BANDWIDTH, rnd(), false), 324566));
  initial.push_back(PrepareBulkData<rtmp::EventSharedObject>(
      rtmp::Header(2, 1, rtmp::EVENT_SHARED_OBJECT, rnd(), false), 20847));
  initial.push_back(new rtmp::EventClientBW(rtmp::Header(2, 1,
      rtmp::EVENT_CLIENT_BANDWIDTH, rnd(), false), 3262434, 23));
  initial.push_back(new rtmp::EventBytesRead(rtmp::Header(2, 1,
      rtmp::EVENT_BYTES_READ, rnd(), false), 20973));
  initial.push_back(new rtmp::EventBytesRead(rtmp::Header(2, 1,
      rtmp::EVENT_BYTES_READ, rnd(), false), 1037));
  initial.push_back(new rtmp::EventBytesRead(rtmp::Header(2, 1,
      rtmp::EVENT_BYTES_READ, rnd(), false), 2734));
  initial.push_back(new rtmp::EventServerBW(rtmp::Header(2, 1,
      rtmp::EVENT_SERVER_BANDWIDTH, rnd(), false), 324566));
  initial.push_back(PrepareBulkData<rtmp::EventVideoData>(
      rtmp::Header(2, 1, rtmp::EVENT_VIDEO_DATA, rnd(), false), 100));
  initial.push_back(new rtmp::EventBytesRead(rtmp::Header(2, 1,
      rtmp::EVENT_BYTES_READ, rnd(), false), 2983));
  initial.push_back(PrepareBulkData<rtmp::EventAudioData>(rtmp::Header(
      2, 1, rtmp::EVENT_AUDIO_DATA, rnd(), false), 261));

  initial.push_back(new rtmp::EventBytesRead(rtmp::Header(2, 1,
      rtmp::EVENT_BYTES_READ, rnd(), false), 18272364));
  initial.push_back(new rtmp::EventServerBW(rtmp::Header(2, 1,
      rtmp::EVENT_SERVER_BANDWIDTH, rnd(), false), 324566));
  initial.push_back(PrepareBulkData<rtmp::EventSharedObject>(
      rtmp::Header(2, 1, rtmp::EVENT_SHARED_OBJECT, rnd(), false), 2183));
  initial.push_back(new rtmp::EventBytesRead(rtmp::Header(2, 1,
      rtmp::EVENT_BYTES_READ, rnd(), false), 208364));

  initial.push_back(PrepareBulkData<rtmp::EventAudioData>(
      rtmp::Header(2, 1, rtmp::EVENT_AUDIO_DATA, rnd(), false), 30283));
  initial.push_back(new rtmp::EventBytesRead(rtmp::Header(2, 1,
      rtmp::EVENT_BYTES_READ, rnd(), false), 1297973));

  initial.push_back(new rtmp::EventBytesRead(rtmp::Header(2, 1,
      rtmp::EVENT_BYTES_READ, rnd(), false), 39743));
  initial.push_back(PrepareBulkData<rtmp::EventFlexSharedObject>(
      rtmp::Header(2, 1, rtmp::EVENT_FLEX_SHARED_OBJECT, rnd(), false), 15));
  initial.push_back(new rtmp::EventBytesRead(rtmp::Header(2, 1,
      rtmp::EVENT_BYTES_READ, rnd(), false), 394573));

  rtmp::EventFlexMessage* const flex = new rtmp::EventFlexMessage(
      rtmp::Header(2, 1, rtmp::EVENT_FLEX_MESSAGE, rnd(), false));
  flex->mutable_data()->push_back(new rtmp::CString("test string"));
  flex->mutable_data()->push_back(new rtmp::CNumber(1234.344));
  initial.push_back(flex);

  initial.push_back(new rtmp::EventBytesRead(rtmp::Header(2, 1,
      rtmp::EVENT_BYTES_READ, rnd(), false), 333));
  initial.push_back(PrepareBulkData<rtmp::EventSharedObject>(
      rtmp::Header(2, 1, rtmp::EVENT_SHARED_OBJECT, rnd(), false), 20847));

  CHECK_LT(FLAGS_fixed_event, static_cast<int32>(initial.size()));
  for ( int i = 0; i < initial.size(); ++i ) {
    rtmp::Header* const h = initial[i]->mutable_header();
    io::MemoryStream tmp;
    initial[i]->EncodeBody(&tmp, rtmp::AmfUtil::AMF0_VERSION);
    scoped_ref<rtmp::Event> p = rtmp::Coder::CreateEvent(*h);
    const rtmp::AmfUtil::ReadStatus err = p->DecodeBody(&tmp,
        rtmp::AmfUtil::AMF0_VERSION);
    CHECK(err == rtmp::AmfUtil::READ_OK) << " Encountered: "
                                         << rtmp::AmfUtil::ReadStatusName(err)
                                         << " ID: " << i
                                         << " for " << *initial[i];
    CHECK(tmp.IsEmpty());
    CHECK(p->Equals(initial[i].get()))
      << "ID: " << i << " Error for: \n" << *initial[i] << "\nvs.\n" << *p;
  }
  {
    rtmp::Coder coder0(4 << 20);
    rtmp::Coder coder1(4 << 20);
    scoped_ref<rtmp::EventAudioData> ev = PrepareBulkData<rtmp::EventAudioData>(
        rtmp::Header(5, 1, rtmp::EVENT_AUDIO_DATA, 0, false), 101);

    io::MemoryStream buf0;

    coder0.Encode(*ev.get(), rtmp::AmfUtil::AMF0_VERSION, &buf0);
    ev->mutable_header()->set_is_timestamp_relative(true);
    ev->mutable_header()->set_timestamp_ms(50);
    coder0.Encode(*ev.get(), rtmp::AmfUtil::AMF0_VERSION, &buf0);
    ev->mutable_header()->set_timestamp_ms(25);
    coder0.Encode(*ev.get(), rtmp::AmfUtil::AMF0_VERSION, &buf0);
    ev->mutable_header()->set_timestamp_ms(50);
    coder0.Encode(*ev.get(), rtmp::AmfUtil::AMF0_VERSION, &buf0);
    coder0.Encode(*ev.get(), rtmp::AmfUtil::AMF0_VERSION, &buf0);
    coder0.Encode(*ev.get(), rtmp::AmfUtil::AMF0_VERSION, &buf0);
    for ( int i = 0; i < 6; ++i ) {
      scoped_ref<rtmp::Event> crt;
      rtmp::AmfUtil::ReadStatus err = coder1.Decode(
          &buf0, rtmp::AmfUtil::AMF0_VERSION, &crt);
      CHECK_EQ(err, rtmp::AmfUtil::READ_OK);
    }
  }

  rtmp::Coder coder(4 << 20);

  deque <scoped_ref<rtmp::Event> > written;
  io::MemoryStream written_buffer;
  io::MemoryStream decode_buffer;

  for ( int i = 0; i < FLAGS_num_ops; i++ ) {
    int rnd_op = random() % 10;
    bool read_op = true;
    if ( written.size() < 1 ) {
      read_op = false;
    } else if ( written.size() < 10 ) {
      read_op = rnd_op >= 7;
    } else if ( written.size() < 20 ) {
      read_op = rnd_op >= 5;
    } else if ( written.size() < 30 ) {
      read_op = rnd_op >= 3;
    }
    if ( read_op ) {
      // transfer some bytes from written_buffer to decode_buffer
      const int64 num_to_transfer =
          min(static_cast<int>((random() % 100 + 1)
                               * coder.read_chunk_size()
                               / (random() % 10 + 1)),
              static_cast<int>(written_buffer.Size()));
      decode_buffer.AppendStream(&written_buffer, num_to_transfer);

      // read as many events as possible from decode_buffer
      while ( true ) {
        scoped_ref<rtmp::Event> event;
        rtmp::AmfUtil::ReadStatus err = coder.Decode(
            &decode_buffer, rtmp::AmfUtil::AMF0_VERSION, &event);
        if ( err == rtmp::AmfUtil::READ_NO_DATA ) {
          break;
        }
        CHECK(err == rtmp::AmfUtil::READ_OK)
            << "Invalid error: " << rtmp::AmfUtil::ReadStatusName(err);
        CHECK(event->Equals(written.front().get()))
          << " Error for: \n" << *written.front() << "\nvs.\n" << *event;
        LOG_INFO << " ===> READ : " << event->ToString();
        written.pop_front();
      }
      continue;
    }

    // write an Event into written_buffer
    const int id = (FLAGS_fixed_event < 0 ? random() % initial.size()
                                          : FLAGS_fixed_event);

    coder.Encode(*initial[id].get(), rtmp::AmfUtil::AMF0_VERSION,
        &written_buffer);
    written.push_back(initial[id]);
    LOG_INFO << " ===> WRIT: " << *initial[id]
             << " NOW: " << written_buffer.Size();
  }
  LOG_INFO << "PASS";
}
