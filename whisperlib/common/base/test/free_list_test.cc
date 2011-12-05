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
#include <stdlib.h>
#include "common/base/types.h"

#include WHISPER_HASH_SET_HEADER

#include "common/base/log.h"
#include "common/base/free_list.h"
#include "common/base/system.h"
#include "common/base/gflags.h"

//////////////////////////////////////////////////////////////////////

DEFINE_int32(num_tests,
             2000,
             "We test w/ these many values");

//////////////////////////////////////////////////////////////////////

void Test(size_t max_size, size_t num_tests) {
  LOG_INFO << " Testing:  max_size = " << max_size
           << " num_tests = " << num_tests;
  util::FreeList<string> strlist(max_size);
  hash_set<string*> expected_content;  // what we expect in free list
  hash_set<string*> allocs;            // keep track of allocations

  for ( size_t i = 0; i < num_tests; i++ ) {
    const int op = random() % 2;
    if ( op || allocs.empty() ) {
      // Alloc
      string* p = strlist.New();
      if ( !expected_content.empty() ) {
        // We expect reuse if available
        const size_t num_erased = expected_content.erase(p);
        CHECK_EQ(num_erased, 1);
      }
      CHECK_EQ(allocs.insert(p).second, true);
    } else {
      // Free - find a random preallocated
      const int num = random() % allocs.size();
      hash_set<string*>::iterator it = allocs.begin();
      for ( int i = 0; i < num; i++ ) {
        ++it;
      }
      // We expect retention if free list not full..
      CHECK_EQ(strlist.Dispose(*it), (expected_content.size() == max_size));
      if ( expected_content.size() < max_size ) {
        // and we track internally the retention
        expected_content.insert(*it);
      }
      allocs.erase(it);
    }
  }
  // In the end we free them all - simply - w / no test
  while ( !allocs.empty() ) {
    strlist.Dispose(*allocs.begin());
    allocs.erase(allocs.begin());
  }
}

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  Test(1, FLAGS_num_tests);
  Test(5, FLAGS_num_tests);
  Test(20, FLAGS_num_tests);
  Test(100, FLAGS_num_tests);
  LOG_INFO << "PASS";
  common::Exit(0);
}
