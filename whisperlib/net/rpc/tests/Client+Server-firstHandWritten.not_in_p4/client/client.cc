
#include <memory>
#include <list>
#include <string>
#include <stdio.h>
#include <stdarg.h>

#include "common/base/types.h"
#include "common/base/errno.h"
#include "common/base/log.h"
#include "common/base/callback/callback2.h"
#include "common/sync/thread.h"
#include "net/rpc/lib/types/rpc_all_types.h"
#include "net/rpc/lib/client/rpc_client_connection_tcp.h"
#include "net/rpc/lib/codec/rpc_codec.h"
#include "net/rpc/lib/codec/binary/rpc_binary_codec.h"
#include "net/rpc/lib/codec/json/rpc_json_codec.h"

#include "../types.h" // custom types
#include "wrappers.h" // service wrappers

class AsyncResultCollectorGigel
{
public:
  bool DoSomethingSuccess_;
  std::string DoSomethingError_;
  rpc::Int DoSomethingRet_;
  synch::Event DoSomethingDone_;
  void DoSomethingResultHandler(int my_int, const char * my_string,
      const rpc::CallResult<rpc::Int> & result) {
    LOG_WARNING << "DoSomethingResultHandler my_int=" << my_int
        << " my_string=" << my_string;
    DoSomethingSuccess_ = result.success_;
    DoSomethingError_ = result.error_;
    DoSomethingRet_ = result.result_;
    DoSomethingDone_.Signal();
  }


  bool DoSomethingElseSuccess_;
  std::string DoSomethingElseError_;
  rpc::Void DoSomethingElseRet_;
  synch::Event DoSomethingElseDone_;
  void DoSomethingElseResultHandler(const rpc::CallResult<rpc::Void> & result)
  {
    DoSomethingElseSuccess_ = result.success_;
    DoSomethingElseError_ = result.error_;
    DoSomethingElseRet_ = result.result_;
    DoSomethingElseDone_.Signal();
  }

  AsyncResultCollectorGigel()
    : DoSomethingSuccess_(false), DoSomethingRet_(), DoSomethingDone_(false, true),
      DoSomethingElseSuccess_(false), DoSomethingElseRet_(), DoSomethingElseDone_(false, true)
  {
  }
};

#define DECLARE_ASYNC_HANDLER(service, method, retType) \
  bool g_success##service##method;\
  std::string g_error##service##method;\
  retType g_ret##service##method;\
  synch::Event g_done##service##method(false, true);\
  void g_AsyncRet##service##method(const rpc::CallResult< retType > & ret)\
  {\
    g_success##service##method = ret.success_;\
    g_error##service##method = ret.error_;\
    g_ret##service##method = ret.result_;\
    g_done##service##method.Signal();\
  }

DECLARE_ASYNC_HANDLER(Mitica, Initialize, rpc::Bool);
DECLARE_ASYNC_HANDLER(Mitica, Exit, rpc::Void);
DECLARE_ASYNC_HANDLER(Mitica, TestMe, rpc::Int32);
DECLARE_ASYNC_HANDLER(Mitica, Foo, rpc::String);
DECLARE_ASYNC_HANDLER(Mitica, SetPerson, rpc::Void);
DECLARE_ASYNC_HANDLER(Mitica, GetPerson, Person);
DECLARE_ASYNC_HANDLER(Mitica, SetFamily, Family);

#define ENCODE_PARAMS0(params_stream) {\
  std::auto_ptr<rpc::Encoder> encoder(codec.CreateEncoder(params_stream));\
  encoder->EncodeArrayStart(0);\
  encoder->EncodeArrayEnd();\
}
#define ENCODE_PARAMS1(params_stream, p1) {\
  std::auto_ptr<rpc::Encoder> encoder(codec.CreateEncoder(params_stream));\
  encoder->EncodeArrayStart(1);\
    encoder->Encode(p1);\
  encoder->EncodeArrayEnd();\
}
#define ENCODE_PARAMS2(params_stream, p1, p2) {\
  std::auto_ptr<rpc::Encoder> encoder(codec.CreateEncoder(params_stream));\
  encoder->EncodeArrayStart(2);\
    encoder->Encode(p1);\
  encoder->EncodeArrayContinue();\
    encoder->Encode(p2);\
  encoder->EncodeArrayEnd();\
}
#define ENCODE_PARAMS3(params_stream, p1, p2, p3) {\
  std::auto_ptr<rpc::Encoder> encoder(codec.CreateEncoder(params_stream));\
  encoder->EncodeArrayStart(3);\
    encoder->Encode(p1);\
  encoder->EncodeArrayContinue();\
    encoder->Encode(p2);\
  encoder->EncodeArrayContinue();\
    encoder->Encode(p3);\
  encoder->EncodeArrayEnd();\
}

int main(int argc, char ** argv)
{
  // - Initialize logger.
  // - Install default signal handlers.
  common::Init(argc, argv);

  ////////////////////////////////////////////
  /*
  Person a;
  a.name_ = rpc::String("abc");
  LOG_INFO << " a = " << a;
  ExitLogger();
  return 0;
  */
  ////////////////////////////////////////////

  const net::HostPort serverAddress("127.0.0.1:5678");

  //rpc::BinaryCodec codec;
  rpc::JsonCodec codec;

  // create selector
  //
  net::Selector selector;

  // create selector's own thread and start it
  //
  thread::Thread selectorThread(NewCallback(&selector, &net::Selector::Loop));
  bool success = selectorThread.SetJoinable();
  CHECK(success) << "selectorThread.SetJoinable() failed: " << GetLastSystemErrorDescription();
  success = selectorThread.Start();
  CHECK(success) << "selectorThread.Start() failed: " << GetLastSystemErrorDescription();

  bool no_error = false;
  do{
    // create RPC connection
    //
    rpc::ClientConnectionTCP rpcConnection(selector, serverAddress, codec.GetCodecID());
    /*
    bool success = rpcConnection.Open();
    if(!success)
    {
      LOG_ERROR << "Failed to connect to RPC server: " << serverAddress;
      continue;
    }
    */

    // TEST negative tests
    //
    LOG_INFO << "############## BEGIN NEGATIVE TESTS ##############";
    {
      rpc::REPLY_STATUS status;
      io::IOMemoryStream result;
      io::IOMemoryStream params;
      ENCODE_PARAMS0(params);

#define TEST_FAIL(cond, errMsg) \
  if(!(cond))\
  {\
    LOG_ERROR << errMsg << " LastSysError: " << GetLastSystemErrorDescription();\
    continue;\
  }\

      // test BAD service
      //
      result.Clear();
      rpcConnection.Query("BlaBla", "blablabla", params, 5000, status, result);
      TEST_FAIL(status == rpc::RPC_SERVICE_UNAVAIL, "Bad status. Expected ServiceUnavailable. Got: " << rpc::ReplyStatusName(status));
      LOG_INFO << "Successfully rpc::Call with bad service.";

      // test BAD method
      //
      result.Clear();
      rpcConnection.Query("Gigel", "fooomethod", params, 5000, status, result);
      TEST_FAIL(status == rpc::RPC_PROC_UNAVAIL, "Bad status. Expected ProcUnavailable. Got: " << rpc::ReplyStatusName(status));
      LOG_INFO << "Successfully rpc::Call with bad global method.";

      result.Clear();
      rpcConnection.Query("Mitica", "fooomethod", params, 5000, status, result);
      TEST_FAIL(status == rpc::RPC_PROC_UNAVAIL, "Bad status. Expected ProcUnavailable. Got: " << rpc::ReplyStatusName(status));
      LOG_INFO << "Successfully rpc::Call with bad global method.";

      // test BAD params
      //
      io::IOMemoryStream badParams;
      ENCODE_PARAMS1(badParams, rpc::String("badParams"));

      result.Clear();
      rpcConnection.Query("Gigel","DoSomething",badParams,5000,status,result);
      TEST_FAIL(status == rpc::RPC_GARBAGE_ARGS, "Bad status. Expected GarbageArgs. Got: " << rpc::ReplyStatusName(status));
      LOG_INFO << "Successfully rpc::Call with bad object params.";

      result.Clear();
      rpcConnection.Query("Gigel","DoSomething",badParams,5000,status,result);
      TEST_FAIL(status == rpc::RPC_GARBAGE_ARGS, "Bad status. Expected GarbageArgs. Got: " << rpc::ReplyStatusName(status));
      LOG_INFO << "Successfully rpc::Call with bad object params.";

      result.Clear();
      rpcConnection.Query("Gigel","DoSomethingElse",badParams,5000,status,result);
      TEST_FAIL(status == rpc::RPC_GARBAGE_ARGS, "Bad status. Expected GarbageArgs. Got: " << rpc::ReplyStatusName(status));
      LOG_INFO << "Successfully rpc::Call with bad object params.";

      result.Clear();
      rpcConnection.Query("Mitica","TestMe",badParams,5000,status,result);
      TEST_FAIL(status == rpc::RPC_GARBAGE_ARGS, "Bad status. Expected GarbageArgs. Got: " << rpc::ReplyStatusName(status));
      LOG_INFO << "Successfully rpc::Call with bad object params.";

      result.Clear();
      rpcConnection.Query("Mitica","TestMe",badParams,5000,status,result);
      TEST_FAIL(status == rpc::RPC_GARBAGE_ARGS, "Bad status. Expected GarbageArgs. Got: " << rpc::ReplyStatusName(status));
      LOG_INFO << "Successfully rpc::Call with bad object params.";

      result.Clear();
      rpcConnection.Query("Mitica","Foo",badParams,5000,status,result);
      TEST_FAIL(status == rpc::RPC_GARBAGE_ARGS, "Bad status. Expected GarbageArgs. Got: " << rpc::ReplyStatusName(status));
      LOG_INFO << "Successfully rpc::Call with bad object params.";

      result.Clear();
      rpcConnection.Query("Mitica","Foo",badParams,5000,status,result);
      TEST_FAIL(status == rpc::RPC_GARBAGE_ARGS, "Bad status. Expected GarbageArgs. Got: " << rpc::ReplyStatusName(status));
      LOG_INFO << "Successfully rpc::Call with bad object params.";
    }

    // TEST Gigel
    //
    LOG_INFO << "############## BEGIN Gigel TESTS ##############";
    {
      rpc::REPLY_STATUS status;
      io::IOMemoryStream result;
      io::IOMemoryStream params;

      // invoke a remote method of Gigel
      //
      {
        params.Clear();
        result.Clear();
        ENCODE_PARAMS0(params);
        rpcConnection.Query("Gigel", "DoSomethingElse", params, 5000, status, result);
        if(status != rpc::RPC_SUCCESS)
        {
          LOG_ERROR << "Failed to call Gigel::DoSomethingElse. Status: "
                    << (success ? rpc::ReplyStatusName(status) : "Transport error.")
                   ;
          continue; // connection is closed on destructor
        }

        // verify returned value
        rpc::Void ret;
        CHECK_EQ(codec.Decode(result, ret), rpc::DECODE_RESULT_SUCCESS);
        LOG_INFO << "Successfully invoked Gigel::DoSomethingElse. Return: " << ret;
      }

      // invoke another remote method of Gigel
      //
      {
        params.Clear();
        result.Clear();

        rpc::Int a = 7;
        rpc::Float b = 2.718282f;
        ENCODE_PARAMS2(params, a, b);
        rpc::Int eRet = 9;
        rpcConnection.Query("Gigel", "DoSomething", params, 5000, status, result);
        if(status != rpc::RPC_SUCCESS)
        {
          LOG_ERROR << "Failed to call Gigel::DoSomething.  Status: "
                    << (success ? rpc::ReplyStatusName(status) : "Transport error.")
                   ;
          continue; // connection is closed on destructor
        }

        // verify returned value
        typeof(eRet) ret;
        CHECK_EQ(codec.Decode(result, ret), rpc::DECODE_RESULT_SUCCESS);
        LOG_INFO << "Successfully invoked Gigel::DoSomething("<<a<<","<<b<<")"
                    " Return: " << ret;
        CHECK(ret == eRet);
      }

      // use a wrapper for Gigel
      //
      {
        ServiceWrapperGigel gigel(rpcConnection);

        {
          rpc::CallResult<rpc::Void> ret;
          gigel.DoSomethingElse(ret);
          if(!ret.success_)
          {
            LOG_ERROR << "Failed to invoke Gigel::DoSomethingElse(): " << ret.error_;
            continue;
          }
          LOG_INFO << "Successfully invoked Gigel::DoSomethingElse()";
        }

        {
          const rpc::Int32 a = 7;
          const rpc::Float b = 2.718282f;
          const rpc::Int32 eRet = 9;
          rpc::CallResult<rpc::Int32> ret;
          gigel.DoSomething(ret, a, b);
          if(!ret.success_)
          {
            LOG_ERROR << "Failed to invoke Gigel::DoSomething(" << a << ", " << b << "): " << ret.error_;
            continue;
          }
          LOG_INFO << "Successfully invoked Gigel::DoSomething(" << a << ", " << b << ") => " << ret.result_;
          CHECK(ret.result_ == eRet);
        }
      }
    }

#define TEST_PASS(service, method, params, expectedRet)\
    {\
      rpc::REPLY_STATUS status;\
      io::IOMemoryStream result;\
      rpcConnection.Query(service, method, params, 5000, status, result);\
      if(status != rpc::RPC_SUCCESS)\
      {\
        LOG_ERROR << "Failed to call "service"::"method".  Status: "\
                  << (success ? rpc::ReplyStatusName(status) : "Transport error.")\
                 ;\
        continue; /* connection is closed on destructor */\
      }\
      \
      /* verify returned value */\
      typeof(expectedRet) ret;\
      CHECK_EQ(codec.Decode(result, ret), rpc::DECODE_RESULT_SUCCESS);\
      CHECK(ret == expectedRet);\
      LOG_INFO << "Successfully invoked "service"::"method"("<<params<<")."\
                  " Return: " << ret;\
    }

    // TEST Mitica
    //
    LOG_INFO << "############## BEGIN Mitica TESTS ##############";
    {
      // create remote instance of Mitica
      //
      io::IOMemoryStream initializeParams;
      rpc::String initializeAddress("Acasa pe stanga");
      ENCODE_PARAMS2(initializeParams, initializeAddress, rpc::Int(169));
      rpc::Bool initializeExpected(true);
      TEST_PASS("Mitica", "Initialize", initializeParams, initializeExpected);

      // invoke a remote method of Mitica
      //
      io::IOMemoryStream testMeParams;
      ENCODE_PARAMS3(testMeParams, rpc::Int(13), rpc::Float(2.718282f), rpc::String("Prepelita ciripeste pe o creanga subreda."));
      rpc::Int32 testMeExpected(15);
      TEST_PASS("Mitica", "TestMe", testMeParams, testMeExpected);

      // invoke another remote method of Mitica
      //
      io::IOMemoryStream fooParams;
      ENCODE_PARAMS1(fooParams, rpc::Date());
      rpc::String & fooExpected = initializeAddress; // the same address should be returned
      TEST_PASS("Mitica", "Foo", fooParams, fooExpected);

      // invoke another remote method of Mitica
      //
      Person person;
      person.name_ = rpc::String("Ionescu");
      person.height_ = rpc::Float(1.76f);
      person.age_ = rpc::Int(123);
      person.married_ = rpc::Bool(true);
      io::IOMemoryStream setPersonParams;
      ENCODE_PARAMS1(setPersonParams, person);
      rpc::Void rpc_void;
      TEST_PASS("Mitica", "SetPerson", setPersonParams, rpc_void);

      // invoke another remote method of Mitica
      //
      io::IOMemoryStream getPersonParams;
      ENCODE_PARAMS0(getPersonParams);
      TEST_PASS("Mitica", "GetPerson", getPersonParams, person);

      // invoke another remote method of Mitica
      //
      Person mother;
      mother.name_.Set(rpc::String("MotherA"));
      mother.height_.Set(rpc::Float(1.68f));
      mother.age_.Set(rpc::Int(103));
      mother.married_.Set(rpc::Bool(true));
      Person father;
      father.name_ = rpc::String("FatherB");
      father.height_ = rpc::Float(1.69f);
      father.age_ = rpc::Int(107);
      father.married_ = rpc::Bool(false);
      rpc::Array<Person> children(2);
      children[0] = mother;
      children[1] = father;
      Family family;
      family.mother_ = mother;
      family.father_ = father;
      family.children_ = children;
      //family.Reference_children_[1] = mother; // modify the second child, so that the return of SetFamily won't be identical with "family"
      io::IOMemoryStream setFamilyParams;
      ENCODE_PARAMS3(setFamilyParams, mother, father, children);
      TEST_PASS("Mitica", "SetFamily", setFamilyParams, family);

      // invoke Exit method of Mitica
      //
      io::IOMemoryStream exitParams;
      ENCODE_PARAMS0(exitParams);
      TEST_PASS("Mitica", "Exit", exitParams, rpc_void);
#undef TEST_PASS

      // use a wrapper for Mitica
      //
      {
        ServiceWrapperMitica mitica(rpcConnection);

#define TEST_PASS(expectedRet, method, args...) {\
  rpc::CallResult< typeof(expectedRet) > result;\
  mitica.method(result , ##args );/*the ## eliminates the last ',' if args are empty*/\
  if(!result.success_)\
  {\
    LOG_ERROR << "Failed to call mitica::" #method ".  Error: " << result.error_;\
    continue; /* connection is closed on destructor */\
  }\
  \
  LOG_INFO << "Successfully invoked mitica::" #method "(...)."\
              " Return: " << result.result_;\
  \
  /* verify returned value */\
  CHECK_EQ(result.result_, expectedRet);\
}
/*
#define TEST_VOID_PASS(method, args...) {\
  bool success = mitica.method( args );\
  if(!success)\
  {\
    LOG_ERROR << "Failed to call mitica::" #method ".  Error: " << mitica.Error();\
    continue;\
  }\
  \
  LOG_INFO << "Successfully invoked mitica::" #method "(...).";\
  \
}
*/
        rpc::String address("Acasa pe stanga");
        rpc::Void void_result;

        // should remember address and return true
        TEST_PASS(rpc::Bool(true), Initialize, address, rpc::Int(169));

        // should sum-up those two numbers
        TEST_PASS(rpc::Int32(15), TestMe, rpc::Int32(13), rpc::Float(2.718282f), rpc::String("Prepelita ciripeste pe o creanga subreda."));
        TEST_PASS(rpc::Int32(12), TestMe, rpc::Int32(10), rpc::Float(2.718282f), rpc::String("Prepelita ciripeste pe o creanga subreda."));

        // should return the address from Initialize
        TEST_PASS(address, Foo, rpc::Date());

        Person person;
        person.name_ = rpc::String("Ionescu");
        person.height_ = rpc::Float(1.76f);
        person.age_ = rpc::Int(123);
        person.married_ = rpc::Bool(true);

        // set person
        TEST_PASS(void_result, SetPerson, person);
        TEST_PASS(void_result, SetPerson, person); // again

        // get person
        TEST_PASS(person, GetPerson );
        TEST_PASS(person, GetPerson ); // again

        Person mother;
        mother.name_ = rpc::String("MotherA");
        mother.height_ = rpc::Float(1.68f);
        mother.age_ = rpc::Int(103);
        mother.married_ = rpc::Bool(true);
        Person father;
        father.name_.Set(rpc::String("FatherB"));
        father.height_.Set(rpc::Float(1.69f));
        father.age_.Set(rpc::Int(107));
        father.married_.Set(rpc::Bool(false));
        rpc::Array<Person> children(2);
        children[0] = mother;
        children[1] = father;
        Family family;
        family.mother_ = mother;
        family.father_ = father;
        family.children_ = children;
        //family.Reference_children()[1] = mother; // modify the second child, so that the return of SetFamily won't be identical with "family"

        // make a Family of the given params and return it.
        TEST_PASS(family, SetFamily, mother, father, children);
        TEST_PASS(family, SetFamily, mother, father, children); // again

        // invoke Exit method of Mitica
        TEST_PASS(void_result, Exit);
      }
    }

    // Asynchronous RPC Gigel
    //
    {
      ServiceWrapperGigel gigel(rpcConnection);
      AsyncResultCollectorGigel res;

      ////////////////////////////
      {
        gigel.DoSomething(NewCallback(&res, &AsyncResultCollectorGigel::DoSomethingResultHandler, 7, "abcdef"),
                               rpc::Int(1), rpc::Float(3.14f));
        if(!res.DoSomethingDone_.Wait(5000))
        {
          LOG_ERROR << "Timeout waiting for DoSomething result";
          break;
        }
        if(!res.DoSomethingSuccess_)
        {
          LOG_ERROR << "An error happened while executing DoSomething: " << res.DoSomethingError_;
          break;
        }
        CHECK_EQ(res.DoSomethingRet_, rpc::Int(4));
        LOG_WARNING << "SUCCESS gigel Async DoSomething";
      }

      ////////////////////////////
      {
        gigel.DoSomethingElse(NewCallback(&res, &AsyncResultCollectorGigel::DoSomethingElseResultHandler));
        if(!res.DoSomethingElseDone_.Wait(5000))
        {
          LOG_ERROR << "Timeout waiting for DoSomethingElse result";
          break;
        }
        if(!res.DoSomethingElseSuccess_)
        {
          LOG_ERROR << "An error happened while executing DoSomethingElse: " << res.DoSomethingElseError_;
          break;
        }
        CHECK_EQ(res.DoSomethingElseRet_, rpc::Void());
        LOG_WARNING << "SUCCESS gigel Async DoSomethingElse";
      }
    }
    // Asynchronous RPC Mitica
    //
    {
      ServiceWrapperMitica wMitica(rpcConnection);

      #define ASYNC_TEST(service, method, expectedReturn, args...) {\
        g_done##service##method.Reset();\
        w##service.method(NewCallback(&g_AsyncRet##service##method), ##args);\
        if(!g_done##service##method.Wait(5000))\
        {\
          LOG_ERROR << "Timeout waiting for " #service "." #method "Async result";\
          break;\
        }\
        if(!g_success##service##method)\
        {\
          LOG_ERROR << "An error happened while executing " #service "." #method "Async: " << g_error##service##method;\
          break;\
        }\
        CHECK_EQ(g_ret##service##method, expectedReturn);\
        LOG_WARNING << "SUCCESS " #service "." #method "Async";\
      }

      rpc::String miticaAddress("Acasa pe stanga");

      ASYNC_TEST(Mitica, Initialize, rpc::Bool(true), rpc::String("adresa lu' mitica"), rpc::Int(7));
      ASYNC_TEST(Mitica, Exit, rpc::Void());
      ASYNC_TEST(Mitica, Initialize, rpc::Bool(true), rpc::String("adresa lu' mitica"), rpc::Int(7));

      // sum up those numbers
      ASYNC_TEST(Mitica, TestMe, rpc::Int32(15), rpc::Int32(13), rpc::Float(2.718282f), rpc::String("Prepelita ciripeste pe o creanga subreda."));
      ASYNC_TEST(Mitica, TestMe, rpc::Int32(12), rpc::Int32(10), rpc::Float(2.718282f), rpc::String("Prepelita ciripeste pe o creanga subreda."));

      // return the address from Initialize
      ASYNC_TEST(Mitica, Foo, rpc::String("adresa lu' mitica"), rpc::Date()); // should return the address from Initialize

      Person person;
      person.name_ = rpc::String("Ionescu");
      person.height_ = rpc::Float(1.76f);
      person.age_ = rpc::Int(123);
      person.married_ = rpc::Bool(true);

      // set person
      ASYNC_TEST(Mitica, SetPerson, rpc::Void(), person);
      ASYNC_TEST(Mitica, SetPerson, rpc::Void(), person);// again

      // get person
      ASYNC_TEST(Mitica, GetPerson, person);
      ASYNC_TEST(Mitica, GetPerson, person); // again

      Person mother;
      mother.name_ = rpc::String("MotherA");
      mother.height_ = rpc::Float(1.68f);
      mother.age_ = rpc::Int(103);
      mother.married_ = rpc::Bool(true);
      Person father;
      father.name_.Set(rpc::String("FatherB"));
      father.height_.Set(rpc::Float(1.69f));
      father.age_.Set(rpc::Int(107));
      father.married_.Set(rpc::Bool(false));
      rpc::Array<Person> children(2);
      children[0] = mother;
      children[1] = father;
      Family family;
      family.mother_ = mother;
      family.father_ = father;
      family.children_ = children;
      //family.Reference_children()[1] = mother; // modify the second child, so that the return of SetFamily won't be identical with "family"

      // make a Family of the given params and return it.
      ASYNC_TEST(Mitica, SetFamily, family, mother, father, children);
      ASYNC_TEST(Mitica, SetFamily, family, mother, father, children); // again

      // invoke Exit method of Mitica
      ASYNC_TEST(Mitica, Exit, rpc::Void());
    }

    // probably not needed: just deleting the connection would suffice
    //rpcConnection.Disconnect();

    // always delete the connection in selector loop
    //selector.DeleteInSelectLoop(&rpcConnection);
    //delete &rpcConnection;

    no_error = true;
  } while(0);

  // stop the selector (this will close & delete the connection too, if not already deleted)
  selector.RunInSelectLoop(NewCallback(&selector, &net::Selector::MakeLoopExit));
  selectorThread.Join();

  LOG_INFO << "Bye.";
  common::Exit(no_error ? 0 : -1);
  return 0;
}
