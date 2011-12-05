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

#include "common/base/types.h"
#include "common/base/errno.h"
#include "common/base/log.h"
#include "net/rpc/lib/types/rpc_all_types.h"
#include "net/rpc/lib/client/irpc_client_connection.h"

#include "auto/types.h"    // for creating transactionable rpc types.

#include "auto/wrappers.h" // helpers. Easy way of calling remote methods.

#define LOG_TEST LOG(-1)

#define DECLARE_ASYNC_GLOBAL_HANDLER(service, method, retType)          \
  bool g_success##service##method;                                      \
  std::string g_error##service##method;                                 \
  retType g_ret##service##method;                                       \
  synch::Event g_done##service##method(false, true);                    \
  void g_AsyncRet##service##method(const rpc::CallResult< retType > & ret) \
  {                                                                     \
    g_success##service##method = ret.success_;                          \
    g_error##service##method = ret.error_;                              \
    g_ret##service##method = ret.result_;                               \
    g_done##service##method.Signal();                                   \
  }

#define DECLARE_ASYNC_MEMBER_HANDLER(method, retType)           \
  bool success##method##_;                                      \
  std::string error##method##_;                                 \
  retType ret##method##_;                                       \
  synch::Event done##method##_;                                 \
  void AsyncRet##method(const rpc::CallResult< retType > & ret) \
  {                                                             \
    success##method##_ = ret.success_;                          \
    error##method##_ = ret.error_;                              \
    ret##method##_ = ret.result_;                               \
    done##method##_.Signal();                                   \
  }

#define INITIALIZE_ASYNC_MEMBER_HANDLER(method)                 \
  success##method##_(false), done##method##_(false, true)

class AsyncResultCollectorGigel {
public:
  DECLARE_ASYNC_MEMBER_HANDLER(DoSomething, int);
  DECLARE_ASYNC_MEMBER_HANDLER(DoSomethingElse, rpc::Void);
  DECLARE_ASYNC_MEMBER_HANDLER(DoDelayReturn, int);

  AsyncResultCollectorGigel()
      : INITIALIZE_ASYNC_MEMBER_HANDLER(DoSomething),
        INITIALIZE_ASYNC_MEMBER_HANDLER(DoSomethingElse),
        INITIALIZE_ASYNC_MEMBER_HANDLER(DoDelayReturn) {}
};
DECLARE_ASYNC_GLOBAL_HANDLER(Gigel, DoSomething, int);
DECLARE_ASYNC_GLOBAL_HANDLER(Gigel, DoSomethingElse, rpc::Void);
DECLARE_ASYNC_GLOBAL_HANDLER(Gigel, DoDelayReturn, int);

class AsyncResultCollectorMitica {
public:
  DECLARE_ASYNC_MEMBER_HANDLER(Initialize, bool);
  DECLARE_ASYNC_MEMBER_HANDLER(Finish, rpc::Void);
  DECLARE_ASYNC_MEMBER_HANDLER(TestMe, int32);
  DECLARE_ASYNC_MEMBER_HANDLER(Foo, string);
  DECLARE_ASYNC_MEMBER_HANDLER(SetPerson, rpc::Void);
  DECLARE_ASYNC_MEMBER_HANDLER(GetPerson, Person);
  DECLARE_ASYNC_MEMBER_HANDLER(SetFamily, Family);

  AsyncResultCollectorMitica()
      : INITIALIZE_ASYNC_MEMBER_HANDLER(Initialize),
      INITIALIZE_ASYNC_MEMBER_HANDLER(Finish),
      INITIALIZE_ASYNC_MEMBER_HANDLER(TestMe),
      INITIALIZE_ASYNC_MEMBER_HANDLER(Foo),
      INITIALIZE_ASYNC_MEMBER_HANDLER(SetPerson),
      INITIALIZE_ASYNC_MEMBER_HANDLER(GetPerson),
      INITIALIZE_ASYNC_MEMBER_HANDLER(SetFamily) {}
};
DECLARE_ASYNC_GLOBAL_HANDLER(Mitica, Initialize, bool);
DECLARE_ASYNC_GLOBAL_HANDLER(Mitica, Finish, rpc::Void);
DECLARE_ASYNC_GLOBAL_HANDLER(Mitica, TestMe, int32);
DECLARE_ASYNC_GLOBAL_HANDLER(Mitica, Foo, string);
DECLARE_ASYNC_GLOBAL_HANDLER(Mitica, SetPerson, rpc::Void);
DECLARE_ASYNC_GLOBAL_HANDLER(Mitica, GetPerson, Person);
DECLARE_ASYNC_GLOBAL_HANDLER(Mitica, SetFamily, Family);

class AsyncResultCollectorGlobal {
public:
  DECLARE_ASYNC_MEMBER_HANDLER(GetTribeByPosition, Tribe);
  DECLARE_ASYNC_MEMBER_HANDLER(AddTribe, bool);
  DECLARE_ASYNC_MEMBER_HANDLER(CreateTribe, Tribe);
  DECLARE_ASYNC_MEMBER_HANDLER(SetTribeHead, int64);
  DECLARE_ASYNC_MEMBER_HANDLER(GetTribeVillager, Villager);
  DECLARE_ASYNC_MEMBER_HANDLER(Clear, rpc::Void);

  AsyncResultCollectorGlobal()
      : INITIALIZE_ASYNC_MEMBER_HANDLER(GetTribeByPosition),
      INITIALIZE_ASYNC_MEMBER_HANDLER(AddTribe),
      INITIALIZE_ASYNC_MEMBER_HANDLER(CreateTribe),
      INITIALIZE_ASYNC_MEMBER_HANDLER(SetTribeHead),
      INITIALIZE_ASYNC_MEMBER_HANDLER(GetTribeVillager),
      INITIALIZE_ASYNC_MEMBER_HANDLER(Clear) {}
};
DECLARE_ASYNC_GLOBAL_HANDLER(Global, GetTribeByPosition, Tribe);
DECLARE_ASYNC_GLOBAL_HANDLER(Global, AddTribe, bool);
DECLARE_ASYNC_GLOBAL_HANDLER(Global, CreateTribe, Tribe);
DECLARE_ASYNC_GLOBAL_HANDLER(Global, SetTribeHead, int64);
DECLARE_ASYNC_GLOBAL_HANDLER(Global, GetTribeVillager, Villager);
DECLARE_ASYNC_GLOBAL_HANDLER(Global, Clear, rpc::Void);

void RunTest(rpc::IClientConnection & rpcConnection) {
  // TEST Gigel
  //
  LOG_TEST << "############## BEGIN Gigel TESTS ##############";
  {
    ServiceWrapperGigel gigel(rpcConnection, "gigel");
    {
      rpc::CallResult<rpc::Void> ret;
      gigel.DoSomethingElse(ret);
      if (!ret.success_) {
        LOG_FATAL << "Failed to invoke Gigel::DoSomethingElse(): "
                  << ret.error_;
      }
      LOG_TEST << "Successfully invoked Gigel::DoSomethingElse()";
    }

    {
      const int32 a = 7;
      const double b = 2.718282f;
      const int32 expected = 9;
      rpc::CallResult<int32> ret;
      gigel.DoSomething(ret, a, b);
      if (!ret.success_) {
        LOG_FATAL << "Failed to invoke Gigel::DoSomething("
                  << a << ", " << b << "): " << ret.error_;
      }
      LOG_TEST << "Successfully invoked Gigel::DoSomething("
               << a << ", " << b << ") => " << ret.result_;
      CHECK(ret.result_ == expected);
    }

    {
      LOG_TEST << "Invoking gigel.DoDelayReturn for SUCCESS";
      const int ms_delay = 2000;
      const int return_value = 3;
      rpc::CallResult<int32> ret;
      gigel.SetCallTimeout(5000);
      gigel.DoDelayReturn(ret, ms_delay, return_value);
      if (!ret.success_) {
        LOG_FATAL << "Failed Gigel::DoDelayReturn("
                  << ms_delay << ", " << return_value << "): " << ret.error_;
      }
      LOG_TEST << "Successfully invoked Gigel::DoDelayReturn("
                  << ms_delay << ", " << return_value << ") => " << ret.result_;
      CHECK(ret.result_ == return_value);
    }

    {
      LOG_TEST << "Invoking gigel.DoDelayReturn for TIMEOUT"
                  " .. this may take a while";
      const int ms_delay = 7000;
      const int return_value = 3;
      rpc::CallResult<int32> ret;
      gigel.SetCallTimeout(5000);
      gigel.DoDelayReturn(ret, ms_delay, return_value);
      if (ret.success_) {
        LOG_FATAL << "Failed Gigel::DoDelayReturn("
                  << ms_delay << ", " << return_value << "):"
                  << " Expected TIMEOUT, got SUCCESS.";
      }
      LOG_TEST << "Successfully invoked Gigel::DoDelayReturn("
               << ms_delay << ", " << return_value << ") with TIMEOUT";
    }
  }

#define TEST_PASS(expectedRet, service, method, args...) {              \
    rpc::CallResult< typeof(expectedRet) > result;                      \
    service.method(result , ##args );                                   \
    if ( !result.success_ ) {                                           \
      LOG_FATAL << "Failed to call "                                    \
          "" #service "::" #method ".  Error: " << result.error_;       \
    }                                                                   \
    LOG_TEST << "Successfully invoked " #service "::" #method "(...)."  \
        " Return: " << result.result_;                                  \
    /* verify returned value */                                         \
    CHECK(result.result_ == expectedRet);                              \
  }

  // TEST Mitica
  //
  LOG_TEST << "############## BEGIN Mitica TESTS ##############";
  {
    ServiceWrapperMitica mitica(rpcConnection, "mitica");

    string address("Acasa pe stanga");
    rpc::Void void_result;

    // should remember address and return true
    TEST_PASS(bool(true), mitica, Initialize, address, int32(169));

    // should sum-up those two numbers
    TEST_PASS(int32(15), mitica, TestMe, int32(13),
              double(2.718282),
              string("Prepelita ciripeste pe o creanga subreda."));

    // should return the address from Initialize
    TEST_PASS(address, mitica, Foo, true);

    Person person;
    person.name_.set(string("Ionescu"));
    person.height_.set(double(1.76));
    person.age_.set(int32(123));
    person.married_.set(bool(true));

    // set person
    TEST_PASS(void_result, mitica, SetPerson, person);

    // get person
    TEST_PASS(person, mitica, GetPerson );

    Person mother;
    mother.name_ = string("MotherA");
    mother.height_ = double(1.68);
    mother.age_ = int32(103);
    mother.married_ = bool(true);
    Person father;
    father.name_ = string("FatherB");
    father.height_ = double(1.69);
    father.age_ = int32(107);
    father.married_ = bool(false);
    vector<Person> children(2);
    children[0] = mother;
    children[1] = father;
    Family family;
    family.mother_ = mother;
    family.father_ = father;
    family.children_ = children;

    // make a Family of the given params and return it.
    TEST_PASS(family, mitica, SetFamily, mother, father, children);

    // return just children of the previous set family
    TEST_PASS(children, mitica, GetChildren );

    // invoke Finish method of Mitica
    TEST_PASS(void_result, mitica, Finish);
  }

  // TEST Tribes
  //
  LOG_TEST << "############## BEGIN Tribe TESTS ##############";
  {
    Tribe nullTribe;
    nullTribe.name_ = string("");
    nullTribe.people_.ref();
    rpc::Void rvoid;
    EmptyType empty_type;

    ServiceWrapperGlobal global(rpcConnection, "global");
    global.SetCallTimeout(120000);

    TEST_PASS(empty_type, global, MirrorEmptyType, empty_type);

    TEST_PASS(rvoid, global, Clear);  // always start with a clean world
    TEST_PASS(nullTribe, global, GetTribeByPosition, int32(1),  int32(3));
    TEST_PASS(nullTribe, global, GetTribeByPosition,
              int32(-1), int32(-3));

    Villager aVillager0;
    aVillager0.ID_ = int64(123);
    aVillager0.name_ = string("a person 123");

    Villager aVillager1;
    aVillager1.ID_ = int64(321);
    aVillager1.name_ = string("a person 321");
    aVillager1.height_ = double(1.7);
    aVillager1.parentID_ = aVillager0.ID_;

    Villager aVillager2;
    aVillager2.ID_ = int64(456);
    aVillager2.name_ = string("a person 456");
    aVillager2.height_ = double(1.9);
    aVillager2.childrenIDs_.ref().push_back(aVillager0.ID_);
    aVillager2.childrenIDs_.ref().push_back(aVillager1.ID_);

    Tribe aTribe;
    aTribe.name_ = string("tribe a");
    map<int64, Villager> & aPeopleMap = aTribe.people_.ref();
    aPeopleMap[aVillager0.ID_.get()] = aVillager0;
    aPeopleMap[aVillager1.ID_.get()] = aVillager1;
    aPeopleMap[aVillager2.ID_.get()] = aVillager2;

    Tribe bTribe;
    bTribe.name_ = string("tribe b");
    bTribe.people_.ref();

    // Add tribe "a"
    TEST_PASS(bool(true ), global, AddTribe, aTribe,
              int32(1), int32(3));
    TEST_PASS(bool(false), global, AddTribe, aTribe,
              int32(1), int32(3));

    // Verify tribe 'a' exists
    TEST_PASS(aTribe, global, GetTribeByPosition, int32(1), int32(3));
    TEST_PASS(nullTribe, global, GetTribeByPosition,
              int32(1),  int32(2));
    TEST_PASS(aTribe, global, GetTribeByPosition, int32(1), int32(3));

    // Verify tribe 'a' persons
    Villager nullVillager;
    nullVillager.ID_ = int64(0);
    nullVillager.name_ = string("");
    TEST_PASS(nullVillager, global, GetTribeVillager,
              string("tribe aa"), aVillager0.ID_.get());
    TEST_PASS(nullVillager, global, GetTribeVillager,
              string("tribe a"), int64(-2));
    TEST_PASS(aVillager0,   global, GetTribeVillager,
              string("tribe a"), aVillager0.ID_.get());
    TEST_PASS(aVillager1,   global, GetTribeVillager,
              string("tribe a"), aVillager1.ID_.get());
    TEST_PASS(aVillager2,   global, GetTribeVillager,
              string("tribe a"), aVillager2.ID_.get());

    // Create a duplicate 'a' tribe.
    vector<Villager> nullPeople;
    TEST_PASS(nullTribe, global, CreateTribe,
              string("tribe new a"), nullPeople,
              int32(1), int32(3));  // position already taken
    TEST_PASS(nullTribe, global, CreateTribe, string("tribe a"),
              nullPeople, int32(1), int32(2));   // name already taken
    vector<Villager> aPeopleArray(3);
    aPeopleArray[0] = aVillager0;
    aPeopleArray[1] = aVillager1;
    aPeopleArray[2] = aVillager2;
    Tribe saTribe = aTribe;
    saTribe.name_ = string("tribe second a");
    TEST_PASS(saTribe, global, CreateTribe,
              string("tribe second a"), aPeopleArray,
              int32(1), int32(2));  // success

    // Verify tribe 'second a' persons
    TEST_PASS(nullVillager, global, GetTribeVillager,
              string("tribe second aa"), aVillager0.ID_.get());
    TEST_PASS(nullVillager, global, GetTribeVillager,
              string("tribe second a"), int64(-2));
    TEST_PASS(aVillager0,   global, GetTribeVillager,
              string("tribe second a"), aVillager0.ID_.get());
    TEST_PASS(aVillager1,   global, GetTribeVillager,
              string("tribe second a"), aVillager1.ID_.get());
    TEST_PASS(aVillager2,   global, GetTribeVillager,
              string("tribe second a"), aVillager2.ID_.get());

    // Add tribe "b"
    TEST_PASS(bool(true ), global, AddTribe, bTribe,
              int32(-2), int32(-5));
    TEST_PASS(bool(false), global, AddTribe, bTribe,
              int32(-2), int32(-5));

    // Verify tribe 'b' exists
    TEST_PASS(bTribe, global, GetTribeByPosition,
              int32(-2), int32(-5));
    TEST_PASS(nullTribe, global, GetTribeByPosition,
              int32(-2),  int32(-2));
    TEST_PASS(bTribe, global, GetTribeByPosition,
              int32(-2), int32(-5));

    // Create a duplicate 'b' tribe.
    TEST_PASS(nullTribe, global, CreateTribe, string("tribe new b"),
              nullPeople,
              int32(-2), int32(-5));  // position already taken
    TEST_PASS(nullTribe, global, CreateTribe, string("tribe b"),
              nullPeople, int32(-2), int32(-3));   // name already taken
    Tribe sbTribe = bTribe;
    sbTribe.name_ = string("tribe second b");
    TEST_PASS(sbTribe, global, CreateTribe, string("tribe second b"),
              nullPeople, int32(-2), int32(-3));   // success

    // set head of tribe 'a'
    TEST_PASS(int64(-1), global, SetTribeHead,
              string("tribe aa"), aVillager0.ID_.get());
    TEST_PASS(int64(-1), global, SetTribeHead,
              string("tribe a"), int64(-1));
    TEST_PASS(int64(0),  global, SetTribeHead,
              string("tribe a"), aVillager0.ID_.get());
    TEST_PASS(aVillager0.ID_.ref(),  global, SetTribeHead,
              string("tribe a"), aVillager1.ID_.get());
    // set it also local
    aTribe.head_id_ = aVillager1.ID_;

    // Verify tribe 'a' correctness
    TEST_PASS(aTribe, global, GetTribeByPosition, int32(1), int32(3));

    // create tribe 'c'
    Villager & cVillager0 = aVillager2;
    Tribe cTribe;
    cTribe.name_ = string("tribe c");
    cTribe.people_.ref()[cVillager0.ID_.get()] = cVillager0;
    vector<Villager> cPeopleArray(1);
    cPeopleArray[0] = cVillager0;
    TEST_PASS(nullTribe, global, CreateTribe, string("tribe a")
              , cPeopleArray, int32(3), int32(3));
    TEST_PASS(nullTribe, global, CreateTribe, string("tribe second b"),
              cPeopleArray, int32(3), int32(3));
    TEST_PASS(cTribe, global, CreateTribe, cTribe.name_.get(), cPeopleArray,
              int32(3), int32(3));

    // set tribe 'c' head
    TEST_PASS(int64(-1), global, SetTribeHead, string("tribe cc"),
              cVillager0.ID_.get());
    TEST_PASS(int64(-1), global, SetTribeHead, string("tribe c"),
              int64(-1));
    TEST_PASS(int64(0),  global, SetTribeHead, string("tribe c"),
              cVillager0.ID_.get());
    TEST_PASS(cVillager0.ID_.ref(),  global, SetTribeHead,
              string("tribe c"), cVillager0.ID_.get());
    // set it also local
    cTribe.head_id_ = cVillager0.ID_;

    // Verify tribe 'c' correctness
    TEST_PASS(cTribe, global, GetTribeByPosition, int32(3), int32(3));
    TEST_PASS(cVillager0, global, GetTribeVillager, string("tribe c"),
              cVillager0.ID_.get());
  }

  //////////////////////////////////////////////////////////////////////
  //                       Asynchronous tests
  //
  // Test Gigel Async
  //
  LOG_TEST << "############## BEGIN Gigel ASYNC TESTS ##############";
  {
    ServiceWrapperGigel gigel(rpcConnection, "gigel");

    //
    // Using a resultCollector class
    //
    AsyncResultCollectorGigel res;
    res.doneDoSomething_.Reset();
    gigel.DoSomething(
        NewCallback(&res, &AsyncResultCollectorGigel::AsyncRetDoSomething),
        int32(1), double(3.14));
    if (!res.doneDoSomething_.Wait(5000)) {
      LOG_FATAL << "Timeout waiting for DoSomething result";
    }
    if (!res.successDoSomething_) {
      LOG_FATAL << "An error happened while executing DoSomething: "
                << res.errorDoSomething_;
    }
    CHECK_EQ(res.retDoSomething_, int32(4));
    LOG_TEST << "SUCCESS gigel Async DoSomething";
    ////////////////////////////
    res.doneDoSomethingElse_.Reset();
    gigel.DoSomethingElse(
        NewCallback(&res, &AsyncResultCollectorGigel::AsyncRetDoSomethingElse));
    if (!res.doneDoSomethingElse_.Wait(5000)) {
      LOG_FATAL << "Timeout waiting for DoSomethingElse result";
    }
    if (!res.successDoSomethingElse_) {
      LOG_FATAL << "An error happened while executing DoSomethingElse: "
                << res.errorDoSomethingElse_;
    }
    CHECK_EQ(res.retDoSomethingElse_, rpc::Void());
    LOG_TEST << "SUCCESS gigel Async DoSomethingElse";
    /////////////////////////////
    res.doneDoDelayReturn_.Reset();
    gigel.DoDelayReturn(
        NewCallback(&res, &AsyncResultCollectorGigel::AsyncRetDoDelayReturn),
        int32(2000), int32(7));
    if (!res.doneDoDelayReturn_.Wait(5000)) {
      LOG_FATAL << "Timeout waiting for DoDelayReturn result";
    }
    if (!res.successDoDelayReturn_) {
      LOG_FATAL << "An error happened while executing DoDelayReturn: "
                << res.errorDoDelayReturn_;
    }
    CHECK_EQ(res.retDoDelayReturn_, int32(7));
    LOG_TEST << "SUCCESS gigel Async DoDelayReturn";
    /////////////////////////////
    res.doneDoDelayReturn_.Reset();
    gigel.SetCallTimeout(3000);
    gigel.DoDelayReturn(
        NewCallback(&res, &AsyncResultCollectorGigel::AsyncRetDoDelayReturn),
        int32(7000), int32(7));
    if (!res.doneDoDelayReturn_.Wait(5000)) {
      LOG_FATAL << "Timeout waiting for DoDelayReturn result";
    }
    if (res.successDoDelayReturn_) {
      LOG_FATAL << "Error DoDelayReturn: expected TIMEOUT, got SUCCESS.";
    }
    gigel.SetCallTimeout(5000);
    LOG_TEST << "SUCCESS gigel Async DoDelayReturn TIMEOUT";
    /////////////////////////////
    res.doneDoDelayReturn_.Reset();
    res.errorDoDelayReturn_ = "";
    res.successDoDelayReturn_ = false;
    rpc::CALL_ID call_id_do_delay_return = gigel.DoDelayReturn(
        NewCallback(&res, &AsyncResultCollectorGigel::AsyncRetDoDelayReturn),
        int32(3000), int32(7));
    gigel.CancelCall(call_id_do_delay_return);
    if (res.doneDoDelayReturn_.Wait(5000)) {
      LOG_FATAL << "Expected timeout waiting for DoDelayReturn result";
    }
    CHECK(!res.successDoDelayReturn_);
    LOG_TEST << "SUCCESS gigel Async DoDelayReturn CANCEL";

    //
    // Using global result handlers
    //
    g_doneGigelDoSomething.Reset();
    gigel.DoSomething(NewCallback(&g_AsyncRetGigelDoSomething), int32(1),
                      double(3.14));
    if (!g_doneGigelDoSomething.Wait(5000)) {
      LOG_FATAL << "Timeout waiting for DoSomethingAsync result";
    }
    if (!g_successGigelDoSomething) {
      LOG_FATAL << "Error DoSomethingAsync: " << g_errorGigelDoSomething;
    }
    CHECK_EQ(g_retGigelDoSomething, int32(4));
    LOG_TEST << "SUCCESS gigel Async DoSomething";
    ////////////////////////////
    g_doneGigelDoSomethingElse.Reset();
    gigel.DoSomethingElse(NewCallback(&g_AsyncRetGigelDoSomethingElse));
    if (!g_doneGigelDoSomethingElse.Wait(5000)) {
      LOG_FATAL << "Timeout waiting for DoSomethingElse result";
    }
    if (!g_successGigelDoSomethingElse) {
      LOG_FATAL << "Error DoSomethingElse: " << g_errorGigelDoSomethingElse;
    }
    LOG_INFO << g_retGigelDoSomethingElse;
    LOG_INFO << rpc::Void();
    CHECK_EQ(g_retGigelDoSomethingElse, rpc::Void());
    LOG_TEST << "SUCCESS gigel Async DoSomethingElse";
    /////////////////////////////
    g_doneGigelDoDelayReturn.Reset();
    gigel.DoDelayReturn(NewCallback(&g_AsyncRetGigelDoDelayReturn),
                        2000, 7);
    if (!g_doneGigelDoDelayReturn.Wait(5000)) {
      LOG_FATAL << "Timeout waiting for DoDelayReturn result";
    }
    if (!g_successGigelDoDelayReturn) {
      LOG_FATAL << "Error DoDelayReturn: " << g_errorGigelDoDelayReturn;
    }
    CHECK_EQ(g_retGigelDoDelayReturn, 7);
    LOG_TEST << "SUCCESS gigel Async DoDelayReturn";
    /////////////////////////////
    g_doneGigelDoDelayReturn.Reset();
    gigel.SetCallTimeout(3000);
    gigel.DoDelayReturn(NewCallback(&g_AsyncRetGigelDoDelayReturn),
                        7000, 7);
    if (!g_doneGigelDoDelayReturn.Wait(5000)) {
      LOG_FATAL << "Timeout waiting for DoDelayReturn result";
    }
    if (g_successGigelDoDelayReturn) {
      LOG_FATAL << "Error DoDelayReturn: expected TIMEOUT, got SUCCESS";
    }
    gigel.SetCallTimeout(5000);
    LOG_TEST << "SUCCESS gigel Async DoDelayReturn TIMEOUT";
    /////////////////////////////
    g_doneGigelDoDelayReturn.Reset();
    g_successGigelDoDelayReturn = false;
    call_id_do_delay_return = gigel.DoDelayReturn(
        NewCallback(&g_AsyncRetGigelDoDelayReturn), 3000, 7);
    gigel.CancelCall(call_id_do_delay_return);
    if (g_doneGigelDoDelayReturn.Wait(5000)) {
      LOG_FATAL << "Expected timeout waiting for DoDelayReturn result";
    }
    CHECK(!g_successGigelDoDelayReturn);
    LOG_TEST << "SUCCESS gigel Async DoDelayReturn CANCEL";
    /////////////////////////////
  }

  // Fasten up a bit the testing process
  //
#define ASYNC_TEST_GLOBAL_CALLBACK(service, method, expectedReturn, args...) { \
    g_done##service##method.Reset();                                    \
    w##service.method(NewCallback(&g_AsyncRet##service##method), ##args); \
    if ( !g_done##service##method.Wait(5000) ) {                        \
      LOG_FATAL << "Timeout waiting for " #service                      \
          "." #method " async result";                                  \
    }                                                                   \
    if ( !g_success##service##method ) {                                \
      LOG_FATAL << "An error happened while executing " #service "."    \
          #method " async: " << g_error##service##method;               \
    }                                                                   \
    CHECK(g_ret##service##method == expectedReturn);                    \
    LOG_TEST << "SUCCESS Async " #service "." #method ;                 \
  }

#define ASYNC_TEST_MEMBER_CALLBACK(service, method, expectedReturn, args...) { \
    rc##service.done##method##_.Reset();                                \
    w##service.method(NewCallback(&rc##service,                         \
            &AsyncResultCollector##service::AsyncRet##method), ##args); \
    if ( !rc##service.done##method##_.Wait(5000) ) {                    \
      LOG_FATAL << "Timeout waiting for " #service                      \
          "." #method " async result";                                  \
    }                                                                   \
    if ( !rc##service.success##method##_ ) {                            \
      LOG_FATAL << "An error happened while executing " #service        \
          "." #method " async: " << rc##service.error##method##_;       \
    }                                                                   \
    CHECK(rc##service.ret##method##_ ==  expectedReturn);               \
    LOG_TEST << "SUCCESS Async " #service "." #method ;                 \
  }

  // Test Mitica Async
  //
  LOG_TEST << "############## BEGIN Mitica ASYNC TESTS ##############";
  {
    // wrapper name must be: w<service>
    ServiceWrapperMitica wMitica(rpcConnection, "mitica");

    Person person;
    person.name_ = string("Ionescu");
    person.height_ = double(1.76);
    person.age_ = int32(123);
    person.married_ = bool(true);

    Person mother;
    mother.name_ = string("MotherA");
    mother.height_ = double(1.68);
    mother.age_ = int32(103);
    mother.married_ = bool(true);
    Person father;
    father.name_.set(string("FatherB"));
    father.height_.set(double(1.69));
    father.age_.set(int32(107));
    father.married_.set(bool(false));
    vector<Person> children(2);
    children[0] = mother;
    children[1] = father;
    Family family;
    family.mother_ = mother;
    family.father_ = father;
    family.children_ = children;

    // by global result callbacks
    ASYNC_TEST_GLOBAL_CALLBACK(Mitica, Initialize, bool(true),
                               string("adresa lu' mitica"), int32(7));
    ASYNC_TEST_GLOBAL_CALLBACK(Mitica, Finish, rpc::Void());
    ASYNC_TEST_GLOBAL_CALLBACK(Mitica, Initialize, bool(true),
                               string("adresa lu' mitica"), int32(7));
    ASYNC_TEST_GLOBAL_CALLBACK(
        Mitica, TestMe, int32(15), int32(13),
        double(2.718282),
        string("Prepelita ciripeste pe o creanga subreda."));
    ASYNC_TEST_GLOBAL_CALLBACK(
        Mitica, TestMe, int32(12),
        int32(10), double(2.718282),
        string("Prepelita ciripeste pe o creanga subreda."));
    ASYNC_TEST_GLOBAL_CALLBACK(
        Mitica, Foo, string("adresa lu' mitica"),
        false);   // should return the address from Initialize
    ASYNC_TEST_GLOBAL_CALLBACK(Mitica, SetPerson, rpc::Void(), person);
    ASYNC_TEST_GLOBAL_CALLBACK(Mitica, SetPerson, rpc::Void(), person);  // x2
    ASYNC_TEST_GLOBAL_CALLBACK(Mitica, GetPerson, person);
    ASYNC_TEST_GLOBAL_CALLBACK(Mitica, GetPerson, person);   // again
    ASYNC_TEST_GLOBAL_CALLBACK(Mitica, SetFamily,
                               family, mother, father, children);
    ASYNC_TEST_GLOBAL_CALLBACK(Mitica, SetFamily,
                               family, mother, father, children);   // again
    ASYNC_TEST_GLOBAL_CALLBACK(Mitica, Finish, rpc::Void());

    // by results collector callbacks
    AsyncResultCollectorMitica rcMitica;
    // results collector's name must be: rc<service>
    ASYNC_TEST_MEMBER_CALLBACK(Mitica, Initialize,
                               bool(true),
                               string("adresa lu' mitica"), int32(7));
    ASYNC_TEST_MEMBER_CALLBACK(Mitica, Finish, rpc::Void());
    ASYNC_TEST_MEMBER_CALLBACK(Mitica, Initialize,
                               bool(true),
                               string("adresa lu' mitica"), int32(7));
    ASYNC_TEST_MEMBER_CALLBACK(
        Mitica, TestMe,
        int32(15), int32(13), double(2.718282),
        string("Prepelita ciripeste pe o creanga subreda."));
    ASYNC_TEST_MEMBER_CALLBACK(
        Mitica, TestMe, int32(12), int32(10), double(2.718282),
        string("Prepelita ciripeste pe o creanga subreda."));
    ASYNC_TEST_MEMBER_CALLBACK(
        Mitica, Foo, string("adresa lu' mitica"),
        true);   // should return the address from Initialize
    ASYNC_TEST_MEMBER_CALLBACK(Mitica, SetPerson, rpc::Void(), person);
    ASYNC_TEST_MEMBER_CALLBACK(Mitica, SetPerson, rpc::Void(), person);  // x2
    ASYNC_TEST_MEMBER_CALLBACK(Mitica, GetPerson, person);
    ASYNC_TEST_MEMBER_CALLBACK(Mitica, GetPerson, person);   // again
    ASYNC_TEST_MEMBER_CALLBACK(Mitica, SetFamily,
                               family, mother, father, children);
    ASYNC_TEST_MEMBER_CALLBACK(Mitica, SetFamily,
                               family, mother, father, children); // again
    ASYNC_TEST_MEMBER_CALLBACK(Mitica, Finish, rpc::Void());
  }

  // Test Tribes Async
  LOG_TEST << "############## BEGIN Tribe ASYNC TESTS ##############";
  {
    Tribe nullTribe;
    nullTribe.name_ = string("");
    nullTribe.people_.ref();

    Villager nullVillager;
    nullVillager.ID_ = int64(0);
    nullVillager.name_ = string("");

    vector<Villager> nullPeople;

    Villager aVillager0;
    aVillager0.ID_ = int64(123);
    aVillager0.name_ = string("a person 123");

    Villager aVillager1;
    aVillager1.ID_ = int64(321);
    aVillager1.name_ = string("a person 321");
    aVillager1.height_ = double(1.7);
    aVillager1.parentID_ = aVillager0.ID_;

    Villager aVillager2;
    aVillager2.ID_ = int64(456);
    aVillager2.name_ = string("a person 456");
    aVillager2.height_ = double(1.9);
    aVillager2.childrenIDs_.ref().push_back(aVillager0.ID_);
    aVillager2.childrenIDs_.ref().push_back(aVillager1.ID_);

    Tribe aTribe;
    aTribe.name_ = string("tribe a");
    map<int64, Villager> & aPeopleMap = aTribe.people_.ref();
    aPeopleMap[aVillager0.ID_.get()] = aVillager0;
    aPeopleMap[aVillager1.ID_.get()] = aVillager1;
    aPeopleMap[aVillager2.ID_.get()] = aVillager2;

    vector<Villager> aPeopleArray(3);
    aPeopleArray[0] = aVillager0;
    aPeopleArray[1] = aVillager1;
    aPeopleArray[2] = aVillager2;

    Tribe saTribe = aTribe;
    saTribe.name_ = string("tribe second a");

    Tribe bTribe;
    bTribe.name_ = string("tribe b");
    bTribe.people_.ref();

    Tribe sbTribe = bTribe;
    sbTribe.name_ = string("tribe second b");

    // create tribe 'c'
    Villager & cVillager0 = aVillager2;
    Tribe cTribe;
    cTribe.name_ = string("tribe c");
    cTribe.people_.ref()[cVillager0.ID_.get()] = cVillager0;
    vector<Villager> cPeopleArray(1);
    cPeopleArray[0] = cVillager0;

    ///////////////////////////////////////////////
    // by global result callbacks
    //
    ServiceWrapperGlobal wGlobal(rpcConnection, "global");

    ASYNC_TEST_GLOBAL_CALLBACK(
        Global, Clear, rpc::Void());   // always start with a clean world
    ASYNC_TEST_GLOBAL_CALLBACK(Global, GetTribeByPosition,
                               nullTribe, int32(1),  int32(3));
    ASYNC_TEST_GLOBAL_CALLBACK(Global, GetTribeByPosition,
                               nullTribe, int32(-1), int32(-3));

    // Add tribe "a"
    ASYNC_TEST_GLOBAL_CALLBACK(
        Global, AddTribe, bool(true ), aTribe, int32(1), int32(3));
    ASYNC_TEST_GLOBAL_CALLBACK(
        Global, AddTribe, bool(false), aTribe, int32(1), int32(3));

    // Verify tribe 'a' exists
    ASYNC_TEST_GLOBAL_CALLBACK(Global, GetTribeByPosition,
                               aTribe, int32(1), int32(3));
    ASYNC_TEST_GLOBAL_CALLBACK(Global, GetTribeByPosition,
                               nullTribe,  int32(1),  int32(2));
    ASYNC_TEST_GLOBAL_CALLBACK(Global, GetTribeByPosition,
                               aTribe, int32(1), int32(3));

    // Verify tribe 'a' persons
    ASYNC_TEST_GLOBAL_CALLBACK(Global, GetTribeVillager, nullVillager,
                               string("tribe aa"), aVillager0.ID_.get());
    ASYNC_TEST_GLOBAL_CALLBACK(Global, GetTribeVillager, nullVillager,
                               string("tribe a"), int64(-2));
    ASYNC_TEST_GLOBAL_CALLBACK(Global, GetTribeVillager, aVillager0,
                               string("tribe a"), aVillager0.ID_.get());
    ASYNC_TEST_GLOBAL_CALLBACK(Global, GetTribeVillager, aVillager1,
                               string("tribe a"), aVillager1.ID_.get());
    ASYNC_TEST_GLOBAL_CALLBACK(Global, GetTribeVillager, aVillager2,
                               string("tribe a"), aVillager2.ID_.get());

    // Create a duplicate 'a' tribe.
    ASYNC_TEST_GLOBAL_CALLBACK(Global, CreateTribe, nullTribe,
                               string("tribe new a"),
                               nullPeople, int32(1),
                               int32(3));   // position already taken
    ASYNC_TEST_GLOBAL_CALLBACK(Global, CreateTribe, nullTribe,
                               string("tribe a"), nullPeople, int32(1),
                               int32(2));   // name already taken
    ASYNC_TEST_GLOBAL_CALLBACK(Global, CreateTribe, saTribe,
                               string("tribe second a"), aPeopleArray,
                               int32(1), int32(2));   // success

    // Verify tribe 'second a' persons
    ASYNC_TEST_GLOBAL_CALLBACK(Global, GetTribeVillager, nullVillager,
                               string("tribe second aa"),
                               aVillager0.ID_.get());
    ASYNC_TEST_GLOBAL_CALLBACK(Global, GetTribeVillager, nullVillager,
                               string("tribe second a"), int64(-2));
    ASYNC_TEST_GLOBAL_CALLBACK(Global, GetTribeVillager,
                               aVillager0,
                               string("tribe second a"),
                               aVillager0.ID_.get());
    ASYNC_TEST_GLOBAL_CALLBACK(Global, GetTribeVillager, aVillager1,
                               string("tribe second a"),
                               aVillager1.ID_.get());
    ASYNC_TEST_GLOBAL_CALLBACK(Global, GetTribeVillager, aVillager2,
                               string("tribe second a"),
                               aVillager2.ID_.get());

    // Add tribe "b"
    ASYNC_TEST_GLOBAL_CALLBACK(Global, AddTribe, bool(true ),
                               bTribe, int32(-2), int32(-5));
    ASYNC_TEST_GLOBAL_CALLBACK(Global, AddTribe, bool(false),
                               bTribe, int32(-2), int32(-5));

    // Verify tribe 'b' exists
    ASYNC_TEST_GLOBAL_CALLBACK(Global, GetTribeByPosition,
                               bTribe, int32(-2), int32(-5));
    ASYNC_TEST_GLOBAL_CALLBACK(Global, GetTribeByPosition,
                               nullTribe,  int32(-2),  int32(-2));
    ASYNC_TEST_GLOBAL_CALLBACK(Global, GetTribeByPosition,
                               bTribe, int32(-2), int32(-5));

    // Create a duplicate 'b' tribe.
    ASYNC_TEST_GLOBAL_CALLBACK(Global, CreateTribe,
                               nullTribe, string("tribe new b"),
                               nullPeople, int32(-2),
                               int32(-5));   // position already taken
    ASYNC_TEST_GLOBAL_CALLBACK(Global, CreateTribe,
                               nullTribe, string("tribe b"), nullPeople,
                               int32(-2),
                               int32(-3));   // name already taken
    ASYNC_TEST_GLOBAL_CALLBACK(Global, CreateTribe, sbTribe,
                               string("tribe second b"), nullPeople,
                               int32(-2), int32(-3));   // success

    // set head of tribe 'a'
    ASYNC_TEST_GLOBAL_CALLBACK(Global, SetTribeHead, int64(-1),
                               string("tribe aa"), aVillager0.ID_.get());
    ASYNC_TEST_GLOBAL_CALLBACK(Global, SetTribeHead, int64(-1),
                               string("tribe a"), int64(-1));
    ASYNC_TEST_GLOBAL_CALLBACK(Global, SetTribeHead, int64(0),
                               string("tribe a"), aVillager0.ID_.get());
    ASYNC_TEST_GLOBAL_CALLBACK(Global, SetTribeHead, aVillager0.ID_.ref(),
                               string("tribe a"), aVillager1.ID_.get());

    // Verify tribe 'a' correctness
    aTribe.head_id_ = aVillager1.ID_;
    // !!! set it also local. Don't forget to roll back!
    ASYNC_TEST_GLOBAL_CALLBACK(Global, GetTribeByPosition, aTribe,
                               int32(1), int32(3));
    aTribe.head_id_ = 0;   // roll back

    // create tribe 'c'
    ASYNC_TEST_GLOBAL_CALLBACK(Global, CreateTribe, nullTribe,
                               string("tribe a"), cPeopleArray,
                               int32(3), int32(3));
    ASYNC_TEST_GLOBAL_CALLBACK(Global, CreateTribe, nullTribe,
                               string("tribe second b"), cPeopleArray,
                               int32(3), int32(3));
    ASYNC_TEST_GLOBAL_CALLBACK(Global, CreateTribe, cTribe, cTribe.name_.get(),
                               cPeopleArray, int32(3), int32(3));

    // set tribe 'c' head
    ASYNC_TEST_GLOBAL_CALLBACK(Global, SetTribeHead, int64(-1),
                               string("tribe cc"), cVillager0.ID_.get());
    ASYNC_TEST_GLOBAL_CALLBACK(Global, SetTribeHead, int64(-1),
                               string("tribe c"), int64(-1));
    ASYNC_TEST_GLOBAL_CALLBACK(Global, SetTribeHead, int64(0),
                               string("tribe c"), cVillager0.ID_.get());
    ASYNC_TEST_GLOBAL_CALLBACK(Global, SetTribeHead, cVillager0.ID_.ref(),
                               string("tribe c"), cVillager0.ID_.get());

    // Verify tribe 'c' correctness
    cTribe.head_id_ = cVillager0.ID_;
    // !!! set it also local. Don't forget to roll back!
    ASYNC_TEST_GLOBAL_CALLBACK(Global, GetTribeByPosition, cTribe,
                               int32(3), int32(3));
    ASYNC_TEST_GLOBAL_CALLBACK(Global, GetTribeVillager, cVillager0,
                               string("tribe c"), cVillager0.ID_.get());
    cTribe.head_id_.reset();   // roll back

    ///////////////////////////////////////////////
    // by member callback results
    //
    AsyncResultCollectorGlobal rcGlobal;
    ASYNC_TEST_MEMBER_CALLBACK(Global, Clear, rpc::Void());
    // always start with a clean world
    ASYNC_TEST_MEMBER_CALLBACK(Global, GetTribeByPosition,
                               nullTribe, int32(1),  int32(3));
    ASYNC_TEST_MEMBER_CALLBACK(Global, GetTribeByPosition,
                               nullTribe, int32(-1), int32(-3));

    // Add tribe "a"
    ASYNC_TEST_MEMBER_CALLBACK(Global, AddTribe, bool(true ),
                               aTribe, int32(1), int32(3));
    ASYNC_TEST_MEMBER_CALLBACK(Global, AddTribe, bool(false),
                               aTribe, int32(1), int32(3));

    // Verify tribe 'a' exists
    ASYNC_TEST_MEMBER_CALLBACK(Global, GetTribeByPosition,
                               aTribe, int32(1), int32(3));
    ASYNC_TEST_MEMBER_CALLBACK(Global, GetTribeByPosition,
                               nullTribe,  int32(1),  int32(2));
    ASYNC_TEST_MEMBER_CALLBACK(Global, GetTribeByPosition,
                               aTribe, int32(1), int32(3));

    // Verify tribe 'a' persons
    ASYNC_TEST_MEMBER_CALLBACK(Global, GetTribeVillager, nullVillager,
                               string("tribe aa"), aVillager0.ID_.get());
    ASYNC_TEST_MEMBER_CALLBACK(Global, GetTribeVillager, nullVillager,
                               string("tribe a"), int64(-2));
    ASYNC_TEST_MEMBER_CALLBACK(Global, GetTribeVillager, aVillager0,
                               string("tribe a"), aVillager0.ID_.get());
    ASYNC_TEST_MEMBER_CALLBACK(Global, GetTribeVillager, aVillager1,
                               string("tribe a"), aVillager1.ID_.get());
    ASYNC_TEST_MEMBER_CALLBACK(Global, GetTribeVillager, aVillager2,
                               string("tribe a"), aVillager2.ID_.get());

    // Create a duplicate 'a' tribe.
    ASYNC_TEST_MEMBER_CALLBACK(Global, CreateTribe, nullTribe,
                               string("tribe new a"), nullPeople,
                               int32(1),
                               int32(3));   // position already taken
    ASYNC_TEST_MEMBER_CALLBACK(Global, CreateTribe, nullTribe,
                               string("tribe a"), nullPeople,
                               int32(1),
                               int32(2));   // name already taken
    ASYNC_TEST_MEMBER_CALLBACK(Global, CreateTribe, saTribe,
                               string("tribe second a"),
                               aPeopleArray,
                               int32(1),
                               int32(2));   // success

    // Verify tribe 'second a' persons
    ASYNC_TEST_MEMBER_CALLBACK(Global, GetTribeVillager, nullVillager,
                               string("tribe second aa"),
                               aVillager0.ID_.get());
    ASYNC_TEST_MEMBER_CALLBACK(Global, GetTribeVillager, nullVillager,
                               string("tribe second a"),
                               int64(-2));
    ASYNC_TEST_MEMBER_CALLBACK(Global, GetTribeVillager, aVillager0,
                               string("tribe second a"),
                               aVillager0.ID_.get());
    ASYNC_TEST_MEMBER_CALLBACK(Global, GetTribeVillager, aVillager1,
                               string("tribe second a"),
                               aVillager1.ID_.get());
    ASYNC_TEST_MEMBER_CALLBACK(Global, GetTribeVillager, aVillager2,
                               string("tribe second a"),
                               aVillager2.ID_.get());

    // Add tribe "b"
    ASYNC_TEST_MEMBER_CALLBACK(Global, AddTribe, bool(true ),
                               bTribe, int32(-2), int32(-5));
    ASYNC_TEST_MEMBER_CALLBACK(Global, AddTribe, bool(false),
                               bTribe, int32(-2), int32(-5));

    // Verify tribe 'b' exists
    ASYNC_TEST_MEMBER_CALLBACK(Global, GetTribeByPosition, bTribe,
                               int32(-2), int32(-5));
    ASYNC_TEST_MEMBER_CALLBACK(Global, GetTribeByPosition, nullTribe,
                               int32(-2),  int32(-2));
    ASYNC_TEST_MEMBER_CALLBACK(Global, GetTribeByPosition, bTribe,
                               int32(-2), int32(-5));

    // Create a duplicate 'b' tribe.
    ASYNC_TEST_MEMBER_CALLBACK(Global, CreateTribe, nullTribe,
                               string("tribe new b"), nullPeople,
                               int32(-2),
                               int32(-5));   // position already taken
    ASYNC_TEST_MEMBER_CALLBACK(Global, CreateTribe, nullTribe,
                               string("tribe b"), nullPeople,
                               int32(-2),
                               int32(-3));   // name already taken
    ASYNC_TEST_MEMBER_CALLBACK(Global, CreateTribe, sbTribe,
                               string("tribe second b"),
                               nullPeople,
                               int32(-2),
                               int32(-3));   // success

    // set head of tribe 'a'
    ASYNC_TEST_MEMBER_CALLBACK(Global, SetTribeHead, int64(-1),
                               string("tribe aa"), aVillager0.ID_.get());
    ASYNC_TEST_MEMBER_CALLBACK(Global, SetTribeHead, int64(-1),
                               string("tribe a"), int64(-1));
    ASYNC_TEST_MEMBER_CALLBACK(Global, SetTribeHead, int64(0),
                               string("tribe a"), aVillager0.ID_.get());
    ASYNC_TEST_MEMBER_CALLBACK(Global, SetTribeHead, aVillager0.ID_.ref(),
                               string("tribe a"), aVillager1.ID_.get());

    // Verify tribe 'a' correctness
    aTribe.head_id_ = aVillager1.ID_;
    // !!! set it also local. Don't forget to roll back!
    ASYNC_TEST_MEMBER_CALLBACK(Global, GetTribeByPosition, aTribe,
                               int32(1), int32(3));
    aTribe.head_id_ = 0;   // roll back

    // create tribe 'c'
    ASYNC_TEST_MEMBER_CALLBACK(Global, CreateTribe, nullTribe,
                               string("tribe a"), cPeopleArray,
                               int32(3), int32(3));
    ASYNC_TEST_MEMBER_CALLBACK(Global, CreateTribe, nullTribe,
                               string("tribe second b"), cPeopleArray,
                               int32(3), int32(3));
    ASYNC_TEST_MEMBER_CALLBACK(Global, CreateTribe, cTribe,
                               cTribe.name_.get(), cPeopleArray,
                               int32(3), int32(3));

    // set tribe 'c' head
    ASYNC_TEST_MEMBER_CALLBACK(Global, SetTribeHead, int64(-1),
                               string("tribe cc"), cVillager0.ID_.get());
    ASYNC_TEST_MEMBER_CALLBACK(Global, SetTribeHead, int64(-1),
                               string("tribe c"), int64(-1));
    ASYNC_TEST_MEMBER_CALLBACK(Global, SetTribeHead, int64(0),
                               string("tribe c"), cVillager0.ID_.get());
    ASYNC_TEST_MEMBER_CALLBACK(Global, SetTribeHead, cVillager0.ID_.ref(),
                               string("tribe c"), cVillager0.ID_.get());

    // Verify tribe 'c' correctness
    cTribe.head_id_ = cVillager0.ID_;
    // !!! set it also local. Don't forget to roll back!
    ASYNC_TEST_MEMBER_CALLBACK(Global, GetTribeByPosition, cTribe,
                               int32(3), int32(3));
    ASYNC_TEST_MEMBER_CALLBACK(Global, GetTribeVillager, cVillager0,
                               string("tribe c"), cVillager0.ID_.get());
    cTribe.head_id_.reset();   // roll back
  }
}
