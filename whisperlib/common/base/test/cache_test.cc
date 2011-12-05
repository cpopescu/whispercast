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

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/common.h>
#include <whisperlib/common/base/system.h>

#include <whisperlib/common/base/cache.h>

uint32 g_cdata_count = 0;

struct CData {
  CData() {
    g_cdata_count++;
  }
  ~CData() {
    g_cdata_count--;
  }
};


void TestLru() {
  util::Cache<uint32, uint32> cache(util::CacheBase::LRU, 3, 5000, NULL, 0);
  cache.Add(1, 1);
  cache.Add(2, 2);
  cache.Add(3, 3);
  cache.Add(4, 4);
  CHECK(cache.Get(1) == 0) << cache.ToString(); // LRU out
  CHECK(cache.Get(2) == 2) << cache.ToString();
  CHECK(cache.Get(3) == 3) << cache.ToString();
  CHECK(cache.Get(4) == 4) << cache.ToString();
}
void TestMru() {
  util::Cache<uint32, uint32> cache(util::CacheBase::MRU, 3, 5000, NULL, 0);
  cache.Add(1, 1);
  cache.Add(2, 2);
  cache.Add(3, 3);
  cache.Add(4, 4);
  CHECK(cache.Get(1) == 1) << cache.ToString();
  CHECK(cache.Get(2) == 2) << cache.ToString();
  CHECK(cache.Get(3) == 0) << cache.ToString(); // MRU out
  CHECK(cache.Get(4) == 4) << cache.ToString();
}
void TestExp() {
  util::Cache<string, string> cache(util::CacheBase::LRU, 3, 100, NULL, "");
  // simple test
  //
  cache.Add("a", "a");
  CHECK(cache.Get("a") == "a") << cache.ToString();
  timer::SleepMsec(110);
  CHECK(cache.Get("a") == "") << cache.ToString(); // 1 expired

  // a more complicated test
  //
  cache.Add("a", "a");
  timer::SleepMsec(30);
  CHECK(cache.Get("a") == "a") << cache.ToString();
  cache.Add("b", "b");
  timer::SleepMsec(30);
  cache.Add("c", "c");
  // "a": 60ms old, live
  // "b": 30ms old, live
  // "c": 0ms old, live
  CHECK(cache.Get("a") == "a") << cache.ToString();
  CHECK(cache.Get("b") == "b") << cache.ToString();
  CHECK(cache.Get("c") == "c") << cache.ToString();
  timer::SleepMsec(50);
  // "a": 110ms old, expired
  // "b": 80ms old, live
  // "c": 50ms old, live
  CHECK(cache.Get("a") == "") << cache.ToString();
  CHECK(cache.Get("b") == "b") << cache.ToString();
  CHECK(cache.Get("c") == "c") << cache.ToString();
  timer::SleepMsec(30);
  // "b": 110ms old, expired
  // "c": 80ms old, live
  CHECK(cache.Get("b") == "") << cache.ToString();
  CHECK(cache.Get("c") == "c") << cache.ToString();
  timer::SleepMsec(30);
  // "c": 110ms old, expired
  CHECK(cache.Get("a") == "") << cache.ToString();
  CHECK(cache.Get("b") == "") << cache.ToString();
  CHECK(cache.Get("c") == "") << cache.ToString();
}
void TestLeak() {
  util::Cache<uint32, const CData*> cache(util::CacheBase::LRU, 3, 1000,
      &util::DefaultValueDestructor<const CData*>, NULL);
  cache.Add(1, new CData());
  cache.Add(2, new CData());
  cache.Add(2, new CData()); // auto deletes the first item with key: 2
  CHECK(cache.Size() == 2) << cache.ToString();
  CHECK(g_cdata_count == 2) << ", g_cdata_count: " << g_cdata_count << cache.ToString();
  cache.Add(3, new CData());
  cache.Add(4, new CData());
  CHECK(cache.Size() == 3) << cache.ToString();
  CHECK(g_cdata_count == 3) << ", g_cdata_count: " << g_cdata_count << cache.ToString();
  cache.Clear();
  CHECK(cache.Size() == 0) << cache.ToString();
  CHECK(g_cdata_count == 0) << ", g_cdata_count: " << g_cdata_count << cache.ToString();
}

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  TestLru();
  TestMru();
  TestExp();
  TestLeak();

  LOG_INFO << "PASS";
  common::Exit(0);
  return 0;
}
