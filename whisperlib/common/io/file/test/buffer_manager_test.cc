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
//
// Tests for BufferManager (buffer_manager.{h,cc})
//
#include <stdio.h>
#include "common/base/types.h"
#include "common/base/log.h"
#include "common/base/timer.h"
#include "common/base/system.h"
#include "common/base/strutil.h"
#include "common/base/gflags.h"
#include "common/base/free_list.h"
#include "common/io/file/buffer_manager.h"
#include "net/base/selector.h"

DEFINE_int32(rand_seed,
             3274,
             "Seed the random number generator w/ this");
DEFINE_int32(num_buffers_to_do,
             1000,
             "How many buffers to run in the test");

DEFINE_int32(min_inter_buf_time_ms,
             10, "Wait at least this long between allocation rounds");
DEFINE_int32(max_inter_buf_time_ms,
             30, "Wait at most this long between allocation rounds");
DEFINE_int32(min_data_produced_time_ms,
             30, "Wait at least this long to produce data");
DEFINE_int32(max_data_produced_time_ms,
             80, "Wait at most this long to produce data");
DEFINE_int32(min_data_use_time_ms,
             60, "How long to use data - minimum");
DEFINE_int32(max_data_use_time_ms,
             300, "How long to use data - maximum");

DEFINE_int32(num_keys,
             75, "Number of keys to consider");
DEFINE_int32(freelist_size,
             50, "Number of available buffers");

static unsigned int rand_seed = 0;
static int num_buffers_to_do = 0;
static int num_active_buffers = 0;


void DataProduced(net::Selector* selector,
                  io::BufferManager::Buffer* buf,
                  int keynum);
void UseData(net::Selector* selector,
             io::BufferManager::Buffer* buf,
             int keynum);
void EndUse(net::Selector* selector,
            io::BufferManager::Buffer* buf);
void StartBufferRequest(net::Selector* selector,
                        io::BufferManager* manager) {
  --num_buffers_to_do;
  if ( (num_buffers_to_do % 100) == 0 ) {
    LOG_INFO << " >>>>>>>>>>>>>>>>>>>>>>>>> TO GO: " << num_buffers_to_do;
  }
  if ( num_buffers_to_do > 0 ) {
    selector->RegisterAlarm(NewCallback(&StartBufferRequest, selector, manager),
                            FLAGS_min_inter_buf_time_ms +
                            rand_r(&rand_seed) % (
                                FLAGS_max_inter_buf_time_ms -
                                FLAGS_min_inter_buf_time_ms));
  } else {
    LOG_INFO << " DONE buffers - active " << num_active_buffers;
  }
  const int keynum = rand_r(&rand_seed) % FLAGS_num_keys;
  const string key = strutil::StringPrintf("KEY-%05d", keynum);

  io::BufferManager::Buffer* buf = manager->GetBuffer(key);
  if ( buf == NULL ) {
    LOG_INFO << " No more buffers left";
    if (num_buffers_to_do <= 0 && num_active_buffers <= 0) {
      selector->MakeLoopExit();
    }
    return;
  }
  num_active_buffers++;
  Closure* full_callback = NewCallback(&UseData,
                                       selector, buf, keynum);
  io::BufferManager::Buffer::State state = buf->BeginUsage(full_callback);
  if ( state == io::BufferManager::Buffer::IN_LOOKUP ) {
    return;   // wait for full_callback to be performed
  }
  delete full_callback;  // not used anymore
  if ( state == io::BufferManager::Buffer::VALID_DATA ) {
    UseData(selector, buf, keynum);
    return;
  }
  *(reinterpret_cast<int*>(buf->data_)) = keynum;
  selector->RegisterAlarm(NewCallback(&DataProduced,
                                      selector, buf, keynum),
                          FLAGS_min_data_produced_time_ms +
                          rand_r(&rand_seed) % (
                              FLAGS_max_data_produced_time_ms -
                              FLAGS_min_data_produced_time_ms));
}
void DataProduced(net::Selector* selector,
                  io::BufferManager::Buffer* buf,
                  int keynum) {
  buf->MarkValidData(buf->data_capacity_);
  UseData(selector, buf, keynum);
}
void UseData(net::Selector* selector,
             io::BufferManager::Buffer* buf,
             int keynum) {
  CHECK_EQ(*(reinterpret_cast<const int*>(buf->data_)), keynum);
  selector->RegisterAlarm(
      NewCallback(&EndUse, selector, buf),
      FLAGS_min_data_use_time_ms +
      rand_r(&rand_seed) % (
          FLAGS_max_data_use_time_ms -
          FLAGS_min_data_use_time_ms));
}
void EndUse(net::Selector* selector,
            io::BufferManager::Buffer* buf) {
  buf->EndUsage(NULL);
  --num_active_buffers;
  if (num_buffers_to_do <= 0 && num_active_buffers <= 0) {
    selector->MakeLoopExit();
  }
}

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  net::Selector selector;
  util::MemAlignedFreeArrayList freelist(8 * sizeof(int32), 16,
                                   FLAGS_freelist_size);
  io::BufferManager manager(&freelist);
  num_buffers_to_do = FLAGS_num_buffers_to_do;

  rand_seed = FLAGS_rand_seed;
  srand(FLAGS_rand_seed);
  selector.RunInSelectLoop(
      NewCallback(&StartBufferRequest, &selector, &manager));
  selector.Loop();
  LOG_INFO << "PASS";
}
