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
#include "common/sync/thread.h"
#include "net/base/address.h"
#include "net/rpc/lib/types/rpc_all_types.h"
#include "net/rpc/lib/client/rpc_client_connection_http.h"
#include "net/rpc/lib/codec/binary/rpc_binary_codec.h"
#include "net/rpc/lib/codec/json/rpc_json_codec.h"
#include "random_creators.h"

#include "auto/types.h"    // for creating transactionable rpc types.

#include "auto/wrappers.h" // helpers. Easy way of calling remote methods.

#define LOG_TEST LOG(-1)

namespace test_http_async {
#define MAX_THREADS 1000 // we allocate an async result array for MAX_THREADS
net::HostPort g_serverAddress;
bool g_end = false;
bool g_performance = false; // true = performance test , false = stress test
uint32 g_nAnswers = 0;
uint32 g_nCalls = 0;
synch::Mutex g_access_nCalls; // to synchronize access to g_nCalls

template <typename T>
class AsyncResult {
public:
  bool success_;
  std::string error_;
  T ret_;
  synch::Event done_;
  AsyncResult() : success_(false), error_(), ret_(), done_(false, true) {}
};

#define DECLARE_ASYNC_GLOBAL_HANDLER(service, method, retType)          \
  AsyncResult<retType>* g_results##service##method = NULL;              \
  void g_AsyncRet##service##method(unsigned nThreadIndex,               \
                                   const rpc::CallResult< retType >& ret) {  \
    LOG_INFO << "TEST RECEIVED g_AsyncRet" #service #method             \
        " for thread #" << nThreadIndex;                                \
    CHECK_LT(nThreadIndex, MAX_THREADS);                                \
    CHECK_NOT_NULL(g_results##service##method);                         \
    AsyncResult<retType>& result = g_results##service##method[nThreadIndex]; \
    result.success_ = ret.success_;                                     \
    result.error_ = ret.error_;                                         \
    result.ret_ = ret.result_;                                          \
    result.done_.Signal();                                              \
    g_nAnswers++;                                                       \
  }

#define INITIALIZE_ASYNC_GLOBAL_HANDLER(service, method, retType)       \
  g_results##service##method = new AsyncResult<retType>[MAX_THREADS];   \

typedef vector<map<string, vector<int32> > >
RPCArrayMapStringArrayInt;

DECLARE_ASYNC_GLOBAL_HANDLER(Service1, MirrorVoid, rpc::Void);
DECLARE_ASYNC_GLOBAL_HANDLER(Service1, MirrorBool, bool);
DECLARE_ASYNC_GLOBAL_HANDLER(Service1, MirrorFloat, double);
DECLARE_ASYNC_GLOBAL_HANDLER(Service1, MirrorInt, int32);
DECLARE_ASYNC_GLOBAL_HANDLER(Service1, MirrorBigInt, int64);
DECLARE_ASYNC_GLOBAL_HANDLER(Service1, MirrorString, string);
DECLARE_ASYNC_GLOBAL_HANDLER(Service1, MirrorArrayOfInt, vector<int32>);
DECLARE_ASYNC_GLOBAL_HANDLER(Service1, MakeArray, vector<string>);
DECLARE_ASYNC_GLOBAL_HANDLER(Service1, MirrorArrayMapStringArrayInt,
                             RPCArrayMapStringArrayInt);

DECLARE_ASYNC_GLOBAL_HANDLER(Service2, MirrorA, A);
DECLARE_ASYNC_GLOBAL_HANDLER(Service2, MirrorB, B);
DECLARE_ASYNC_GLOBAL_HANDLER(Service2, MirrorC, C);
DECLARE_ASYNC_GLOBAL_HANDLER(Service2, MirrorD, D);
DECLARE_ASYNC_GLOBAL_HANDLER(Service2, BuildA, A);
DECLARE_ASYNC_GLOBAL_HANDLER(Service2, BuildB, B);
DECLARE_ASYNC_GLOBAL_HANDLER(Service2, BuildC, C);
DECLARE_ASYNC_GLOBAL_HANDLER(Service2, BuildArrayC, vector<C>);
DECLARE_ASYNC_GLOBAL_HANDLER(Service2, BuildD, D);

void InitializeGlobalHandlers() {
  INITIALIZE_ASYNC_GLOBAL_HANDLER(Service1, MirrorVoid, rpc::Void);
  INITIALIZE_ASYNC_GLOBAL_HANDLER(Service1, MirrorBool, bool);
  INITIALIZE_ASYNC_GLOBAL_HANDLER(Service1, MirrorFloat, double);
  INITIALIZE_ASYNC_GLOBAL_HANDLER(Service1, MirrorInt, int32);
  INITIALIZE_ASYNC_GLOBAL_HANDLER(Service1, MirrorBigInt, int64);
  INITIALIZE_ASYNC_GLOBAL_HANDLER(Service1, MirrorString, string);
  INITIALIZE_ASYNC_GLOBAL_HANDLER(Service1, MirrorArrayOfInt,
                                  vector<int32>);
  INITIALIZE_ASYNC_GLOBAL_HANDLER(Service1, MakeArray, vector<string>);
  INITIALIZE_ASYNC_GLOBAL_HANDLER(Service1, MirrorArrayMapStringArrayInt,
                                  RPCArrayMapStringArrayInt);

  INITIALIZE_ASYNC_GLOBAL_HANDLER(Service2, MirrorA, A);
  INITIALIZE_ASYNC_GLOBAL_HANDLER(Service2, MirrorB, B);
  INITIALIZE_ASYNC_GLOBAL_HANDLER(Service2, MirrorC, C);
  INITIALIZE_ASYNC_GLOBAL_HANDLER(Service2, MirrorD, D);
  INITIALIZE_ASYNC_GLOBAL_HANDLER(Service2, BuildA, A);
  INITIALIZE_ASYNC_GLOBAL_HANDLER(Service2, BuildB, B);
  INITIALIZE_ASYNC_GLOBAL_HANDLER(Service2, BuildC, C);
  INITIALIZE_ASYNC_GLOBAL_HANDLER(Service2, BuildArrayC, vector<C>);
  INITIALIZE_ASYNC_GLOBAL_HANDLER(Service2, BuildD, D);
}

#define ASYNC_TEST_START(service, method, args...) {                    \
    g_results##service##method[nThreadIndex].done_.Reset();             \
    w##service.method(NewCallback(&g_AsyncRet##service##method,         \
                                  nThreadIndex), ##args);               \
    LOG_INFO << "TEST_START launched Async call " #service "." #method  \
        " on thread #" << nThreadIndex;                                 \
    synch::MutexLocker lock(&g_access_nCalls);                          \
    g_nCalls++;                                                         \
  }

#define ASYNC_TEST_END(service, method, expectedReturn) {               \
    LOG_INFO << "TEST_END waiting on Async call completion "            \
        #service "." #method " on thread #" << nThreadIndex;            \
    AsyncResult<typeof expectedReturn>&                                 \
        result = g_results##service##method[nThreadIndex];              \
    if(!result.done_.Wait(20000)) {                                     \
      LOG_ERROR << "Timeout waiting for Async call completion"          \
          #service "." #method " on thread #" << nThreadIndex;          \
      /* break */;                                                      \
    } else if ( !result.success_ ) {                                    \
      LOG_ERROR << "An error happened while executing Async call "      \
          #service "." #method " on thread #" << nThreadIndex           \
                << " : " << result.error_;                              \
      break;                                                            \
    } else {                                                            \
      CHECK_EQ(result.ret_, expectedReturn); }                          \
  }

void ClientThread(net::Selector* s, unsigned nThreadIndex) {
  CHECK_NOT_NULL(s);
  LOG_INFO << "Thread #" << nThreadIndex << " running...";

  // rpc::BinaryCodec codec;
  rpc::JsonCodec codec;

  while ( !g_end ) {
    const int nRand = Random();
    bool rpcBool( (nRand % 2)  == 0 );
    double rpcFloat( RandomFloat() );
    int32 rpcInt( (int32)nRand );
    int64 rpcBigInt( Random64() );
    string rpcString( RandomString(nRand % 128) );
    vector<int32> rpcArrayInt = RandomArrayInt(nRand % 128);
    vector<string> rpcArrayString(3);
    rpcArrayString[0] = RandomString( Random() % 128 );
    rpcArrayString[1] = string("b"); // RandomString( Random() % 128 );
    rpcArrayString[2] = string("c"); // RandomString( Random() % 128 );
    vector< map< string, vector< int32 > > >
    RPCArrayMapStringArrayInt(3);
    RPCArrayMapStringArrayInt[0][string(RandomString(Random() % 8))] =
      RandomArrayInt(nRand % 128);
    RPCArrayMapStringArrayInt[0][string(RandomString(Random() % 8))] =
      RandomArrayInt(nRand % 128);
    RPCArrayMapStringArrayInt[0][string(RandomString(Random() % 8))] =
      RandomArrayInt(nRand % 128);
    RPCArrayMapStringArrayInt[1][string(RandomString(Random() % 8))] =
      RandomArrayInt(nRand % 128);
    RPCArrayMapStringArrayInt[1][string(RandomString(Random() % 8))] =
      RandomArrayInt(nRand % 128);
    RPCArrayMapStringArrayInt[1][string(RandomString(Random() % 8))] =
      RandomArrayInt(nRand % 128);
    RPCArrayMapStringArrayInt[2][string(RandomString(Random() % 8))] =
      RandomArrayInt(nRand % 128);
    RPCArrayMapStringArrayInt[2][string(RandomString(Random() % 8))] =
      RandomArrayInt(nRand % 128);
    RPCArrayMapStringArrayInt[2][string(RandomString(Random() % 8))] =
      RandomArrayInt(nRand % 128);

    A a = RandomA();
    B b = RandomB();
    C c;
    c.a_ = a;
    c.b_ = b;
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
    vector<A> arrayA(nSize);
    vector<B> arrayB(nSize);
    vector<C> arrayC(nSize);
    for (unsigned i = 0; i < nSize; i++) {
      arrayA[i] = RandomA();
      arrayB[i] = RandomB();
      arrayC[i].a_ = arrayA[i];
      arrayC[i].b_ = arrayB[i];
    }
    D d2;
    d2.a_ = arrayA;
    d2.b_ = arrayB;
    d2.c_ = arrayC;

    // create rpc:: connection
    //
    http::ClientParams http_client_params;
    http_client_params.dlog_level_ = false;
    http_client_params.max_header_size_ = 256;
    http_client_params.max_body_size_ = 10 << 20;
    http_client_params.max_chunk_size_ = 1024;
    http_client_params.max_num_chunks_ = 5;
    http_client_params.default_request_timeout_ms_ = 4000;
    http_client_params.connect_timeout_ms_ = 1000;
    http_client_params.write_timeout_ms_ = 1000;
    http_client_params.read_timeout_ms_ = 1000;

    net::NetFactory net_factory(s);
    rpc::ClientConnectionHTTP rpcConnection(
        *s, net_factory, net::PROTOCOL_TCP,
        http_client_params, g_serverAddress, codec.GetCodecID(), "/rpc");

    // create rpc:: service wrappers
    //
    ServiceWrapperService1 wService1(rpcConnection, "service1");
    ServiceWrapperService2 wService2(rpcConnection, "service2");

    // the performance test ! The answers are countered, not the calls.
    while ( g_performance && !g_end ) {
      ASYNC_TEST_START(Service1, MirrorVoid);
      ASYNC_TEST_START(Service1, MirrorBool, rpcBool);
      ASYNC_TEST_START(Service1, MirrorFloat, rpcFloat);
      ASYNC_TEST_START(Service1, MirrorInt, rpcInt);
      ASYNC_TEST_START(Service1, MirrorBigInt, rpcBigInt);
      ASYNC_TEST_START(Service1, MirrorString, rpcString);
      ASYNC_TEST_START(Service1, MirrorArrayOfInt, rpcArrayInt);
      ASYNC_TEST_START(Service1, MakeArray,
                       rpcArrayString[0], rpcArrayString[1], rpcArrayString[2]);
      ASYNC_TEST_START(Service1, MirrorArrayMapStringArrayInt,
                       RPCArrayMapStringArrayInt);

      ASYNC_TEST_START(Service2, MirrorA, a);
      ASYNC_TEST_START(Service2, MirrorB, b);
      ASYNC_TEST_START(Service2, MirrorC, c);
      ASYNC_TEST_START(Service2, MirrorD, d);
      ASYNC_TEST_START(Service2, BuildA, a.value_.get());
      ASYNC_TEST_START(Service2, BuildB, b.bBool_.get(), b.names_.get());
      ASYNC_TEST_START(Service2, BuildC, a, b);
      ASYNC_TEST_START(Service2, BuildArrayC, arrayA, arrayB);
      ASYNC_TEST_START(Service2, BuildD, arrayA, arrayB);

      //::usleep(g_nCalls - g_nAnswers);
      ASYNC_TEST_END(Service1, MirrorVoid, rpc::Void());
      ASYNC_TEST_END(Service1, MirrorBool, rpcBool);
      ASYNC_TEST_END(Service1, MirrorFloat, rpcFloat);
      ASYNC_TEST_END(Service1, MirrorInt, rpcInt);
      ASYNC_TEST_END(Service1, MirrorBigInt, rpcBigInt);
      ASYNC_TEST_END(Service1, MirrorString, rpcString);
      ASYNC_TEST_END(Service1, MirrorArrayOfInt, rpcArrayInt);
      ASYNC_TEST_END(Service1, MakeArray, rpcArrayString);
      ASYNC_TEST_END(Service1, MirrorArrayMapStringArrayInt,
                     RPCArrayMapStringArrayInt);

      ASYNC_TEST_END(Service2, MirrorA, a);
      ASYNC_TEST_END(Service2, MirrorB, b);
      ASYNC_TEST_END(Service2, MirrorC, c);
      ASYNC_TEST_END(Service2, MirrorD, d);
      ASYNC_TEST_END(Service2, BuildA, a);
      ASYNC_TEST_END(Service2, BuildB, b);
      ASYNC_TEST_END(Service2, BuildC, c);
      ASYNC_TEST_END(Service2, BuildArrayC, arrayC);
      ASYNC_TEST_END(Service2, BuildD, d2);
    }
    if ( g_end ) {
      break;
    }

    switch ( Random() % 5 ) {
    case 0: // stop in the same order
      ASYNC_TEST_START(Service1, MirrorVoid);
      ASYNC_TEST_START(Service1, MirrorBool, rpcBool);
      ASYNC_TEST_START(Service1, MirrorFloat, rpcFloat);
      ASYNC_TEST_START(Service1, MirrorInt, rpcInt);
      ASYNC_TEST_START(Service1, MirrorBigInt, rpcBigInt);
      ASYNC_TEST_START(Service1, MirrorString, rpcString);
      ASYNC_TEST_START(Service1, MirrorArrayOfInt, rpcArrayInt);
      ASYNC_TEST_START(Service1, MakeArray,
                       rpcArrayString[0],
                       rpcArrayString[1],
                       rpcArrayString[2]);
      ASYNC_TEST_START(Service1, MirrorArrayMapStringArrayInt,
                       RPCArrayMapStringArrayInt);

      ASYNC_TEST_END(Service1, MirrorVoid, rpc::Void());
      ASYNC_TEST_END(Service1, MirrorBool, rpcBool);
      ASYNC_TEST_END(Service1, MirrorFloat, rpcFloat);
      ASYNC_TEST_END(Service1, MirrorInt, rpcInt);
      ASYNC_TEST_END(Service1, MirrorBigInt, rpcBigInt);
      ASYNC_TEST_END(Service1, MirrorString, rpcString);
      ASYNC_TEST_END(Service1, MirrorArrayOfInt, rpcArrayInt);
      ASYNC_TEST_END(Service1, MakeArray, rpcArrayString);
      ASYNC_TEST_END(Service1, MirrorArrayMapStringArrayInt,
                     RPCArrayMapStringArrayInt);
      break;
    case 1: // stop in reverse order
      ASYNC_TEST_START(Service2, MirrorA, a);
      ASYNC_TEST_START(Service2, MirrorB, b);
      ASYNC_TEST_START(Service2, MirrorC, c);
      ASYNC_TEST_START(Service2, MirrorD, d);
      ASYNC_TEST_START(Service2, BuildA, a.value_.get());
      ASYNC_TEST_START(Service2, BuildB, b.bBool_.get(), b.names_.get());
      ASYNC_TEST_START(Service2, BuildC, a, b);
      ASYNC_TEST_START(Service2, BuildArrayC, arrayA, arrayB);
      ASYNC_TEST_START(Service2, BuildD, arrayA, arrayB);

      ASYNC_TEST_END(Service2, BuildD, d2);
      ASYNC_TEST_END(Service2, BuildArrayC, arrayC);
      ASYNC_TEST_END(Service2, BuildC, c);
      ASYNC_TEST_END(Service2, BuildB, b);
      ASYNC_TEST_END(Service2, BuildA, a);
      ASYNC_TEST_END(Service2, MirrorD, d);
      ASYNC_TEST_END(Service2, MirrorC, c);
      ASYNC_TEST_END(Service2, MirrorB, b);
      ASYNC_TEST_END(Service2, MirrorA, a);
      break;
    case 2: // stop in random order
      ASYNC_TEST_START(Service1, MirrorBool, rpcBool);
      ASYNC_TEST_START(Service1, MirrorFloat, rpcFloat);
      ASYNC_TEST_END(Service1, MirrorBool, rpcBool);
      ASYNC_TEST_START(Service1, MirrorInt, rpcInt);
      ASYNC_TEST_END(Service1, MirrorInt, rpcInt);
      ASYNC_TEST_END(Service1, MirrorFloat, rpcFloat);
      ASYNC_TEST_START(Service2, BuildA, a.value_.get());
      ASYNC_TEST_START(Service2, BuildB, b.bBool_.get(), b.names_.get());
      ASYNC_TEST_START(Service2, BuildArrayC, arrayA, arrayB);
      ASYNC_TEST_END(Service2, BuildB, b);
      ASYNC_TEST_START(Service2, BuildD, arrayA, arrayB);
      ASYNC_TEST_END(Service2, BuildArrayC, arrayC);
      ASYNC_TEST_END(Service2, BuildD, d2);
      ASYNC_TEST_END(Service2, BuildA, a);
      break;
    case 3: // stop in random order, forget about some
      ASYNC_TEST_START(Service1, MirrorVoid);
      ASYNC_TEST_START(Service2, MirrorC, c);
      ASYNC_TEST_START(Service1, MirrorBool, rpcBool);
      ASYNC_TEST_START(Service1, MirrorInt, rpcInt);
      ASYNC_TEST_START(Service1, MirrorBigInt, rpcBigInt);
      ASYNC_TEST_START(Service2, BuildA, a.value_.get());
      ASYNC_TEST_START(Service1, MirrorString, rpcString);
      ASYNC_TEST_START(Service2, BuildC, a, b);
      ASYNC_TEST_START(Service1, MakeArray,
                       rpcArrayString[0],
                       rpcArrayString[1],
                       rpcArrayString[2]);
      ASYNC_TEST_START(Service2, BuildD, arrayA, arrayB);
      ASYNC_TEST_START(Service2, MirrorA, a);
      ASYNC_TEST_START(Service2, MirrorB, b);
      ASYNC_TEST_START(Service1, MirrorFloat, rpcFloat);
      ASYNC_TEST_START(Service2, MirrorD, d);
      ASYNC_TEST_START(Service1, MirrorArrayOfInt, rpcArrayInt);
      ASYNC_TEST_START(Service2, BuildB, b.bBool_.get(), b.names_.get());
      ASYNC_TEST_START(Service1, MirrorArrayMapStringArrayInt,
                       RPCArrayMapStringArrayInt);
      ASYNC_TEST_START(Service2, BuildArrayC, arrayA, arrayB);

      ASYNC_TEST_END(Service1, MakeArray, rpcArrayString);
      ASYNC_TEST_END(Service1, MirrorInt, rpcInt);
      ASYNC_TEST_END(Service1, MirrorBool, rpcBool);
      // ASYNC_TEST_END(Service2, MirrorA, a);
      ASYNC_TEST_END(Service2, BuildA, a);
      ASYNC_TEST_END(Service1, MirrorString, rpcString);
      // ASYNC_TEST_END(Service1, MirrorBigInt, rpcBigInt);
      ASYNC_TEST_END(Service2, BuildC, c);
      ASYNC_TEST_END(Service2, MirrorC, c);
      // ASYNC_TEST_END(Service2, MirrorD, d);
      ASYNC_TEST_END(Service1, MirrorVoid, rpc::Void());
      ASYNC_TEST_END(Service1, MirrorArrayMapStringArrayInt,
                     RPCArrayMapStringArrayInt);
      // ASYNC_TEST_END(Service2, BuildArrayC, arrayC);
      ASYNC_TEST_END(Service2, MirrorB, b);
      ASYNC_TEST_END(Service1, MirrorFloat, rpcFloat);
      ASYNC_TEST_END(Service2, BuildD, d2);
      // ASYNC_TEST_END(Service1, MirrorArrayOfInt, rpcArrayInt);
      ASYNC_TEST_END(Service2, BuildB, b);
      break;
    case 4: // don't stop at all
      ASYNC_TEST_START(Service1, MirrorVoid);
      ASYNC_TEST_START(Service1, MirrorBool, rpcBool);
      ASYNC_TEST_START(Service1, MirrorFloat, rpcFloat);
      ASYNC_TEST_START(Service1, MirrorInt, rpcInt);
      ASYNC_TEST_START(Service1, MirrorBigInt, rpcBigInt);
      ASYNC_TEST_START(Service1, MirrorString, rpcString);
      ASYNC_TEST_START(Service1, MirrorArrayOfInt, rpcArrayInt);
      ASYNC_TEST_START(Service1, MakeArray,
                       rpcArrayString[0],
                       rpcArrayString[1],
                       rpcArrayString[2]);
      ASYNC_TEST_START(Service1, MirrorArrayMapStringArrayInt,
                       RPCArrayMapStringArrayInt);
      ASYNC_TEST_START(Service2, MirrorA, a);
      ASYNC_TEST_START(Service2, MirrorB, b);
      ASYNC_TEST_START(Service2, MirrorC, c);
      ASYNC_TEST_START(Service2, MirrorD, d);
      ASYNC_TEST_START(Service2, BuildA, a.value_.get());
      ASYNC_TEST_START(Service2, BuildB, b.bBool_.get(), b.names_.get());
      ASYNC_TEST_START(Service2, BuildC, a, b);
      ASYNC_TEST_START(Service2, BuildArrayC, arrayA, arrayB);
      ASYNC_TEST_START(Service2, BuildD, arrayA, arrayB);
      break;
    default:
      LOG_FATAL << "Not such case";
    };
  }
  LOG_INFO << "Thread #" << nThreadIndex << " stopped.";
}

void run(unsigned nThreads,
         unsigned nSeconds,
         const net::HostPort& serverAddress) {
  CHECK_LT(nThreads, MAX_THREADS);
  LOG_TEST << "Begin HTTP asynchronous test";
  g_serverAddress = serverAddress;

  InitializeGlobalHandlers();

  // create selector
  //
  net::Selector selector;

  // create selector's own thread and start it
  //
  thread::Thread selectorThread(NewCallback(&selector, &net::Selector::Loop));
  CHECK(selectorThread.SetJoinable());
  CHECK(selectorThread.Start());

  thread::Thread ** threads = new thread::Thread*[nThreads];
  CHECK_NOT_NULL(threads);

  /////////////////////////////////////////////////////////////////

  g_performance = false;
  g_end = false;
  g_nCalls = 0;
  g_nAnswers = 0;

  const size_t stack_size = size_t(PTHREAD_STACK_MIN) + 65536 * 4;

  LOG_TEST << "+Starting the ASYNC stress test with " << nThreads
  << " threads.";
  for ( unsigned i = 0; i < nThreads; i++ ) {
    threads[i] = new thread::Thread(
      NewCallback(&test_http_async::ClientThread, &selector, i));
    CHECK_NOT_NULL(threads[i]);
    CHECK(threads[i]->SetJoinable());
    CHECK(threads[i]->SetStackSize(stack_size));
    CHECK(threads[i]->Start());
  }

  LOG_TEST << " Stress test running for " << nSeconds << " seconds.";
  ::sleep(nSeconds);

  LOG_TEST << " Times UP. Waiting all threads...";
  g_end = true;
  for ( unsigned i = 0; i < nThreads; i++ ) {
    threads[i]->Join();
    delete threads[i];
    threads[i] = NULL;
  }
  LOG_TEST << " All threads stopped. " << g_nCalls
  << " calls made, " << g_nAnswers << " answers received.";

  /////////////////////////////////////////////////////////////////

  g_performance = true;
  g_end = false;
  g_nCalls = 0;
  g_nAnswers = 0;

  LOG_TEST << "+Starting the ASYNC performance test with "
  << nThreads << " threads.";
  for ( unsigned i = 0; i < nThreads; i++ ) {
    threads[i] = new thread::Thread(
      NewCallback(&test_http_async::ClientThread, &selector, i));
    CHECK_NOT_NULL(threads[i]);
    CHECK(threads[i]->SetJoinable());
    CHECK(threads[i]->SetStackSize(stack_size));
    CHECK(threads[i]->Start());
  }

  LOG_TEST << " Performance test running for " << nSeconds << " seconds.";
  ::sleep(nSeconds);

  LOG_TEST << " Times UP. Waiting all threads...";
  g_end = true;
  for (unsigned i = 0; i < nThreads; i++) {
    threads[i]->Join();
    delete threads[i];
    threads[i] = NULL;
  }
  LOG_TEST << " All threads stopped. " << g_nCalls
  << " calls made, " << g_nAnswers << " answers received.";
  LOG_TEST << " Performance: " << (static_cast<double>(g_nCalls) /
                                   static_cast<double>(nSeconds))
  << " calls/second.";

  for (unsigned i = 0; i < nThreads; i++) {
    delete threads[i];
    threads[i] = NULL;
  }
  threads = NULL;

  // stop the selector (this will close & delete the connection too,
  // if not already deleted)
  selector.RunInSelectLoop(
    NewCallback(&selector, &net::Selector::MakeLoopExit));
  selectorThread.Join();

  LOG_TEST << "HTTP asynchronous test terminated";
}

};
