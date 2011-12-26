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
#include <whisperstreamlib/rtmp/rtmp_protocol_data.h>

//////////////////////////////////////////////////////////////////////

DEFINE_int32(num_ops,
             10000,
             " Number of operations in the test");
DEFINE_int32(fixed_event,
             -1,
             "Write just one type of events..");

//////////////////////////////////////////////////////////////////////

template<class C>
C* PrepareBulkData(int size, rtmp::Header* header) {
  C* ret = new C(header);
  for ( int i = 0; i < size; i++ ) {
    const int x = random();
    ret->mutable_data()->Write(&x, sizeof(x));
  }
  return ret;
}


int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  vector <scoped_ref<rtmp::Event> > initial;
  vector <bool> is_bulk;
  rtmp::ProtocolData protocol;

  initial.push_back(new rtmp::EventChunkSize(256,
                                             new rtmp::Header(&protocol)));
  is_bulk.push_back(false);
  initial.push_back(new rtmp::EventBytesRead(123,
                                             new rtmp::Header(&protocol)));
  is_bulk.push_back(false);
  initial.push_back(new rtmp::EventServerBW(324566,
                                            new rtmp::Header(&protocol)));
  is_bulk.push_back(false);
  initial.push_back(PrepareBulkData<rtmp::EventSharedObject>(
                      20847, new rtmp::Header(&protocol)));
  is_bulk.push_back(true);
  initial.push_back(new rtmp::EventClientBW(3262434, 23,
                                            new rtmp::Header(&protocol)));
  is_bulk.push_back(false);
  initial.push_back(new rtmp::EventBytesRead(20973,
                                             new rtmp::Header(&protocol)));
  is_bulk.push_back(false);
  initial.push_back(new rtmp::EventBytesRead(
                        1037, new rtmp::Header(&protocol)));
  is_bulk.push_back(false);
  initial.push_back(new rtmp::EventBytesRead(
                        2734, new rtmp::Header(&protocol)));
  is_bulk.push_back(false);
  initial.push_back(new rtmp::EventServerBW(324566,
                                            new rtmp::Header(&protocol)));
  is_bulk.push_back(false);
  initial.push_back(PrepareBulkData<rtmp::EventVideoData>(
                      100, new rtmp::Header(&protocol)));
  is_bulk.push_back(true);
  initial.push_back(new rtmp::EventBytesRead(2983,
                                             new rtmp::Header(&protocol)));
  is_bulk.push_back(false);
  initial.push_back(PrepareBulkData<rtmp::EventAudioData>(
                      261, new rtmp::Header(&protocol)));
  is_bulk.push_back(true);

  initial.push_back(new rtmp::EventBytesRead(18272364,
                                             new rtmp::Header(&protocol)));
  is_bulk.push_back(false);
  initial.push_back(new rtmp::EventServerBW(324566,
                                            new rtmp::Header(&protocol)));
  is_bulk.push_back(false);
  initial.push_back(PrepareBulkData<rtmp::EventSharedObject>(
                      2183, new rtmp::Header(&protocol)));
  is_bulk.push_back(true);
  initial.push_back(new rtmp::EventBytesRead(208364,
                                             new rtmp::Header(&protocol)));
  is_bulk.push_back(false);

  initial.push_back(PrepareBulkData<rtmp::EventAudioData>(
                      30283, new rtmp::Header(&protocol)));
  is_bulk.push_back(true);
  initial.push_back(new rtmp::EventBytesRead(1297973,
                                             new rtmp::Header(&protocol)));
  is_bulk.push_back(false);

  initial.push_back(new rtmp::EventBytesRead(39743,
                                             new rtmp::Header(&protocol)));
  is_bulk.push_back(false);
  initial.push_back(PrepareBulkData<rtmp::EventFlexSharedObject>(
                      156, new rtmp::Header(&protocol)));
  is_bulk.push_back(true);
  initial.push_back(new rtmp::EventBytesRead(394573,
                                             new rtmp::Header(&protocol)));
  is_bulk.push_back(false);

  rtmp::EventFlexMessage* const flex =
    new rtmp::EventFlexMessage(new rtmp::Header(&protocol));
  flex->mutable_data()->push_back(new rtmp::CString("test string"));
  flex->mutable_data()->push_back(new rtmp::CNumber(1234.344));
  initial.push_back(flex);
  is_bulk.push_back(false);

  initial.push_back(new rtmp::EventBytesRead(333,
                                             new rtmp::Header(&protocol)));
  is_bulk.push_back(false);
  initial.push_back(PrepareBulkData<rtmp::EventSharedObject>(
                      20847, new rtmp::Header(&protocol)));
  is_bulk.push_back(true);

  CHECK_LT(FLAGS_fixed_event, static_cast<int32>(initial.size()));
  CHECK_EQ(initial.size(), is_bulk.size());
  for ( int i = 0; i < initial.size(); ++i ) {
    rtmp::Header* const h = initial[i]->mutable_header();
    io::MemoryStream tmp;

    h->set_stream_id(1);  // avoid notigy ..
    h->set_channel_id(2);
    h->set_event_type(initial[i]->event_type());
    h->set_timestamp_ms(random() % 0xffffff);
    initial[i]->WriteToMemoryStream(&tmp, rtmp::AmfUtil::AMF0_VERSION);
    h->set_event_size(tmp.Size());
    scoped_ref<rtmp::Event> p = rtmp::Coder::CreateEvent(
        new rtmp::Header(&protocol, *h));
    if ( i == 12 ) {
      LOG_INFO << " Break Here !!!";
    }
    const rtmp::AmfUtil::ReadStatus err =
        p->ReadFromMemoryStream(&tmp, rtmp::AmfUtil::AMF0_VERSION);
    CHECK(err == rtmp::AmfUtil::READ_OK) << " Encountered: "
                                         << rtmp::AmfUtil::ReadStatusName(err)
                                         << " ID: " << i
                                         << " for " << *initial[i];
    CHECK(tmp.IsEmpty());
    CHECK(p->Equals(initial[i].get()))
      << "ID: " << i << " Error for: \n" << *initial[i] << "\nvs.\n" << *p;
  }
  {
    rtmp::Coder coder0(&protocol, 4 << 20);
    rtmp::Coder coder1(&protocol, 4 << 20);
    scoped_ref<rtmp::EventAudioData> ev = PrepareBulkData<rtmp::EventAudioData>(
        101, new rtmp::Header(&protocol));
    ev->mutable_header()->set_stream_id(1);  // avoid notify ..
    ev->mutable_header()->set_channel_id(5);
    ev->mutable_header()->set_event_type(ev->event_type());
    ev->mutable_header()->set_timestamp_ms(0);
    ev->mutable_header()->set_is_timestamp_relative(false);
    ev->mutable_header()->set_event_size(ev->data().Size());

    io::MemoryStream buf0;

    coder0.Encode(&buf0, rtmp::AmfUtil::AMF0_VERSION, ev.get());
    ev->mutable_header()->set_is_timestamp_relative(true);
    ev->mutable_header()->set_timestamp_ms(50);
    coder0.Encode(&buf0, rtmp::AmfUtil::AMF0_VERSION, ev.get());
    ev->mutable_header()->set_timestamp_ms(25);
    coder0.Encode(&buf0, rtmp::AmfUtil::AMF0_VERSION, ev.get());
    ev->mutable_header()->set_timestamp_ms(50);
    coder0.Encode(&buf0, rtmp::AmfUtil::AMF0_VERSION, ev.get());
    coder0.Encode(&buf0, rtmp::AmfUtil::AMF0_VERSION, ev.get());
    coder0.Encode(&buf0, rtmp::AmfUtil::AMF0_VERSION, ev.get());
    for ( int i = 0; i < 6; ++i ) {
      scoped_ref<rtmp::Event> crt;
      rtmp::AmfUtil::ReadStatus err = coder1.Decode(
          &buf0,
          rtmp::AmfUtil::AMF0_VERSION,
          &crt);
      CHECK_EQ(err, rtmp::AmfUtil::READ_OK);
    }
  }

  rtmp::Coder coder(&protocol, 4 << 20);

  deque <scoped_ref<rtmp::Event> > written;
  deque <bool> written_is_bulk;
  io::MemoryStream written_buffer;
  io::MemoryStream decode_buffer;

  for ( int i = 0; i < FLAGS_num_ops; i++ ) {
    int rnd_op = random() % 10;
    bool read_op;
    if ( written.size() < 1 ) {
      read_op = false;
    } else if ( written.size() < 10 ) {
      read_op = rnd_op >= 7;
    } else if ( written.size() < 20 ) {
      read_op = rnd_op >= 5;
    } else if ( written.size() < 30 ) {
      read_op = rnd_op >= 3;
    } else {
      read_op = true;
    }
    if ( read_op ) {
      const int64 num_to_transfer =
          min(static_cast<int>((random() % 100 + 1)
                               * protocol.read_chunk_size()
                               / (random() % 10 + 1)),
              static_cast<int>(written_buffer.Size()));
      decode_buffer.AppendStreamNonDestructive(&written_buffer,
                                               num_to_transfer);
      CHECK_EQ(written_buffer.Skip(num_to_transfer), num_to_transfer);

      while ( true ) {
        scoped_ref<rtmp::Event> event;
        rtmp::AmfUtil::ReadStatus err = coder.Decode(
            &decode_buffer,
            rtmp::AmfUtil::AMF0_VERSION,
            &event);
        if ( err == rtmp::AmfUtil::READ_NO_DATA ) {
          break;
        }
        CHECK(err == rtmp::AmfUtil::READ_OK)
            << "Invalid error: " << rtmp::AmfUtil::ReadStatusName(err);
        CHECK(event->Equals(written.front().get()))
          << " Error for: \n" << *written.front() << "\nvs.\n" << *event;
        LOG_INFO << " ===> READ : " << event->ToString();
        written.pop_front();
        written_is_bulk.pop_front();
      };
    } else {
      int id = (FLAGS_fixed_event < 0
                ? random() % initial.size()
                : FLAGS_fixed_event);

      coder.Encode(&written_buffer, rtmp::AmfUtil::AMF0_VERSION,
                   initial[id].get());
      written.push_back(initial[id]);
      written_is_bulk.push_back(is_bulk[id]);
      LOG_INFO << " ===> WRIT: " << *initial[id]
               << " NOW: " << written_buffer.Size();
    }
  }
  LOG_INFO << "PASS";
}
