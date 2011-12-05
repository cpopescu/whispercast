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
#include <string>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>

#include "common/base/types.h"
#include "common/base/errno.h"
#include "common/base/log.h"
#include "common/base/timer.h"
#include "common/sync/thread.h"
#include "net/rpc/lib/types/rpc_all_types.h"
#include "net/rpc/lib/client/rpc_client_connection_tcp.h"
#include "net/rpc/lib/codec/rpc_codec.h"
#include "net/rpc/lib/codec/binary/rpc_binary_codec.h"
#include "net/rpc/lib/codec/json/rpc_json_codec.h"
#include "random_creators.h"

#include "auto/types.h"    // for creating transactionable rpc types.

#include "auto/wrappers.h" // helpers. Easy way of calling remote methods.

namespace test_tcp_sync {

net::HostPort g_serverAddress;
bool g_end = false;
uint64 g_nCalls = 0;
synch::Mutex sync_nCalls;

void CallsIncrement() {
  synch::MutexLocker lock(&sync_nCalls);
  g_nCalls++;
}

struct ClientThreadArgs {
  uint32 nLoopCount_;
  uint32 nThreadIndex_;
  ClientThreadArgs(uint32 nLoopCount, uint32 nThreadIndex)
      : nLoopCount_(nLoopCount), nThreadIndex_(nThreadIndex) {
  }
};

void ClientThread(net::Selector* selector,
                  net::NetFactory* net_factory,
                 uint32 nThreadIndex,
                 uint32 nLoopCount) {
  LOG_INFO << "Thread #" << nThreadIndex << " started.";

  while(!g_end) {
    rpc::JsonCodec codec;

    // create RPC connection
    //
    rpc::ClientConnectionTCP rpcConnection(
        *selector, *net_factory, net::PROTOCOL_TCP,
        g_serverAddress, codec.GetCodecID());
    bool success = rpcConnection.Open();
    if (!success) {
      LOG_ERROR << "Failed to connect to RPC server: " << g_serverAddress
      << " error: " << rpcConnection.Error();
      continue;
    }

#define TEST_METHOD(service, expectedRet, method, args...) {            \
      rpc::CallResult< typeof(expectedRet) > result;                    \
      service.method(result , ##args );                                 \
      if ( !result.success_ ) {                                         \
        LOG_ERROR << "Failed to call " #service "::" #method ".  Error: " \
                  << result.error_;                                     \
        break;                                                          \
      } else {                                                          \
        LOG_INFO << "Successfully invoked " #service "::" #method "(...)." \
            " Return: " << result.result_;                              \
        /* verify returned value */                                     \
        CHECK_EQ(result.result_, expectedRet);                          \
        CallsIncrement();                                               \
      }                                                                 \
    }

    for (uint32 i = 0; i < nLoopCount && !g_end; ++i) {
      LOG_INFO << "Client Loop #" << i;
      // TEST service1
      //
      LOG_INFO << "############## BEGIN Service1 TESTS ##############";
      {
        ServiceWrapperService1 service1(rpcConnection, "service1");
        service1.SetCallTimeout(20000);

        const int nRand = Random();

        rpc::Void rpcVoid;
        TEST_METHOD(service1, rpcVoid, MirrorVoid );

        bool rpcBool( (nRand % 2)  == 0 );
        TEST_METHOD(service1, rpcBool, MirrorBool, rpcBool);

        double rpcFloat( RandomFloat() );
        TEST_METHOD(service1, rpcFloat, MirrorFloat, rpcFloat);

        int32 rpcInt( (int32)nRand );
        TEST_METHOD(service1, rpcInt, MirrorInt, rpcInt);

        int64 rpcBigInt( Random64() );
        TEST_METHOD(service1, rpcBigInt, MirrorBigInt, rpcBigInt);

        string rpcString( RandomString(nRand % 128) );
        TEST_METHOD(service1, rpcString, MirrorString, rpcString);

        vector<int32> rpcArrayInt = RandomArrayInt(nRand % 128);
        TEST_METHOD(service1, rpcArrayInt, MirrorArrayOfInt, rpcArrayInt);

        vector<string> rpcArrayString(3);
        rpcArrayString[0] = RandomString( Random() % 128 );
        rpcArrayString[1] = string("b");//RandomString( Random() % 128 );
        rpcArrayString[2] = string("c");//RandomString( Random() % 128 );
        TEST_METHOD(service1, rpcArrayString, MakeArray,
                    rpcArrayString[0], rpcArrayString[1], rpcArrayString[2]);

        vector< map< string, vector< int32 > > >
            rpcArrayMapStringArrayInt(3);
        rpcArrayMapStringArrayInt[0][string(RandomString(Random() % 8))] =
            RandomArrayInt(nRand % 128);
        rpcArrayMapStringArrayInt[0][string(RandomString(Random() % 8))] =
            RandomArrayInt(nRand % 128);
        rpcArrayMapStringArrayInt[0][string(RandomString(Random() % 8))] =
            RandomArrayInt(nRand % 128);
        rpcArrayMapStringArrayInt[1][string(RandomString(Random() % 8))] =
            RandomArrayInt(nRand % 128);
        rpcArrayMapStringArrayInt[1][string(RandomString(Random() % 8))] =
            RandomArrayInt(nRand % 128);
        rpcArrayMapStringArrayInt[1][string(RandomString(Random() % 8))] =
            RandomArrayInt(nRand % 128);
        rpcArrayMapStringArrayInt[2][string(RandomString(Random() % 8))] =
            RandomArrayInt(nRand % 128);
        rpcArrayMapStringArrayInt[2][string(RandomString(Random() % 8))] =
            RandomArrayInt(nRand % 128);
        rpcArrayMapStringArrayInt[2][string(RandomString(Random() % 8))] =
            RandomArrayInt(nRand % 128);
        TEST_METHOD(service1, rpcArrayMapStringArrayInt,
                    MirrorArrayMapStringArrayInt, rpcArrayMapStringArrayInt);
      }

      // TEST service2
      //
      LOG_INFO << "############## BEGIN Service2 TESTS ##############";
      {
        ServiceWrapperService2 service2(rpcConnection, "service2");
        service2.SetCallTimeout(60000);

        A a = RandomA();
        TEST_METHOD(service2, a, MirrorA, a);

        B b = RandomB();
        TEST_METHOD(service2, b, MirrorB, b);

        C c;
        c.a_ = a;
        c.b_ = b;
        TEST_METHOD(service2, c, MirrorC, c);

        D d;
        const unsigned nSize = (Random() % 5);
        vector<A>& array_a = d.a_.ref();
        vector<B>& array_b = d.b_.ref();
        vector<C>& array_c = d.c_.ref();
        array_a.resize(nSize);
        array_b.resize(nSize);
        array_c.resize(nSize);
        for (unsigned i = 0; i < nSize; i++) {
          array_a[i] = RandomA();
          array_b[i] = RandomB();
          array_c[i].a_ = array_a[i];
          array_c[i].b_ = array_b[i];
        }
        TEST_METHOD(service2, d, MirrorD, d);

        TEST_METHOD(service2, a, BuildA, a.value_.get());

        TEST_METHOD(service2, b, BuildB, b.bBool_.get(), b.names_.get());

        TEST_METHOD(service2, c, BuildC, a, b);

        vector<A> arrayA(nSize);
        vector<B> arrayB(nSize);
        vector<C> arrayC(nSize);
        for (unsigned i = 0; i < nSize; i++) {
          arrayA[i] = RandomA();
          arrayB[i] = RandomB();
          arrayC[i].a_ = arrayA[i];
          arrayC[i].b_ = arrayB[i];
        }
        TEST_METHOD(service2, arrayC, BuildArrayC, arrayA, arrayB);

        D d2;
        d2.a_ = arrayA;
        d2.b_ = arrayB;
        d2.c_ = arrayC;
        TEST_METHOD(service2, d2, BuildD, arrayA, arrayB);
      }
    }
  };

  LOG_INFO << "Thread #" << nThreadIndex << " terminated.";
}

void run(unsigned nThreads, unsigned nSeconds,
         const net::HostPort& serverAddress) {
  LOG(-1) << "Begin TCP synchronous test";
  g_serverAddress = serverAddress;

  // create selector
  //
  net::Selector selector;

  // create selector's own thread and start it
  //
  thread::Thread selectorThread(NewCallback(&selector, &net::Selector::Loop));
  bool success = selectorThread.SetJoinable();
  CHECK(success) << "selectorThread.SetJoinable() failed: "
                 << GetLastSystemErrorDescription();
  success = selectorThread.Start();
  CHECK(success) << "selectorThread.Start() failed: "
                 << GetLastSystemErrorDescription();

  // create net_factory
  //
  net::NetFactory net_factory(&selector);

  thread::Thread ** threads = new thread::Thread*[nThreads];
  CHECK_NOT_NULL(threads);
  const size_t stack_size = size_t(PTHREAD_STACK_MIN) + 4 * 65536;

  //////////////////////////////////////////////////////////////////////////

  {
    LOG(-1) << "+Starting the SYNC stress test with " << nThreads << " threads.";
    g_end = false;
    g_nCalls = 0;

    for ( unsigned i = 0; i < nThreads; i++ ) {
      uint32 nRandLoops = Random() % 10;
      threads[i] = new thread::Thread(
        NewCallback(&test_tcp_sync::ClientThread, &selector, &net_factory, i, nRandLoops));
      CHECK_NOT_NULL(threads[i]);
      CHECK(threads[i]->SetStackSize(stack_size));
      CHECK(threads[i]->SetJoinable());
      CHECK(threads[i]->Start());
    }
    LOG(-1) << "Random connections test running for "
            << nSeconds << " seconds on " << nThreads << " threads.";

    ::sleep(nSeconds);

    LOG_INFO << " Times UP. Waiting all threads...";
    g_end = true;
    for ( unsigned i = 0; i < nThreads; i++ ) {
      threads[i]->Join();
      delete threads[i];
      threads[i] = NULL;
    }
    LOG(-1) << " All threads stopped. " << g_nCalls << " calls made.";
  }

  //////////////////////////////////////////////////////////////////////////

  {
    LOG(-1) << "+Starting the SYNC full connections test.";
    g_end = false;
    g_nCalls = 0;

    for ( unsigned i = 0; i < nThreads; i++ ) {
      uint32 nRandLoops = 999999;
      threads[i] = new thread::Thread(
        NewCallback(&test_tcp_sync::ClientThread, &selector, &net_factory, i, nRandLoops));
      CHECK_NOT_NULL(threads[i]);
      CHECK(threads[i]->SetStackSize(stack_size));
      CHECK(threads[i]->SetJoinable());
      CHECK(threads[i]->Start());
    }
    LOG(-1) << " All connections running for " << nSeconds
            << " seconds on " << nThreads << " threads.";

    ::sleep(nSeconds);

    LOG(-1) << " Time's UP, terminating...";
    g_end = true;
    for ( unsigned i = 0; i < nThreads; i++ ) {
      threads[i]->Join();
      delete threads[i];
      threads[i] = NULL;
    }
    LOG(-1) << " All threads stopped. " << g_nCalls << " calls made.";

    LOG(-1) << "Performance: " << ((double)g_nCalls) / ((double)nSeconds)
            << " calls/second.";
  }

  delete [] threads;
  threads = NULL;

  // stop the selector (this will close & delete the connection too,
  // if not already deleted)
  selector.RunInSelectLoop(
      NewCallback(&selector, &net::Selector::MakeLoopExit));
  selectorThread.Join();

  LOG(-1) << "TCP synchronous test terminated";
  LOG(-1) << "";
}

};
