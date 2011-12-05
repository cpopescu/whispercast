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

#ifndef __NET_RPC_TEST_SAMPLE_SERVER_SERVICES_IMPL_H__
#define __NET_RPC_TEST_SAMPLE_SERVER_SERVICES_IMPL_H__

#include <map>
#include <string>
#include <whisperlib/net/base/selector.h>
#include <whisperlib/net/rpc/tests/Client-Server-sample/server/auto/types.h>
#include <whisperlib/net/rpc/tests/Client-Server-sample/server/auto/invokers.h>

class RPCServiceGigel
      : public ServiceInvokerGigel {
  net::SelectorThread selector_thread_;
 public:
  RPCServiceGigel(const string& instance_name);
  virtual ~RPCServiceGigel();

 protected:
  void DoSomething(rpc::CallContext<int32>* call, int32 a, double b);
  void DoSomethingElse(rpc::CallContext<rpc::Void>* call);
  void DoDelayReturn(rpc::CallContext<int32>* call,
                     int32 ms_delay, int32 return_value);

 private:
  void DelayReturn(rpc::CallContext<int32>* call, int32 return_value);
};

class RPCServiceMitica : public ServiceInvokerMitica {
  // Local variables are not defined in services.rpc .
  std::string address_;
  uint8 age_;
  Person person_;
  Family family_;

 public:
  RPCServiceMitica(const string& instance_name);
  virtual ~RPCServiceMitica();

 protected:
  void Initialize(rpc::CallContext<bool>* call,
                  const string& address, int32 age);
  void Finish(rpc::CallContext<rpc::Void>* call);
  void TestMe(rpc::CallContext<int32>* call,
              int32 a, double b, const string& c);
  void Foo(rpc::CallContext<string>* call, bool x);
  void SetPerson(rpc::CallContext<rpc::Void>* call, const Person& arg0);
  void GetPerson(rpc::CallContext<Person>* call);
  void SetFamily(rpc::CallContext<Family>* call,
                 const Person& mother,
                 const Person& father,
                 const vector< Person >& children);
  void GetChildren(rpc::CallContext<vector<Person> >* call);
};

class RPCServiceGlobal : public ServiceInvokerGlobal {
  struct Position {
    int x_;
    int y_;

    static const char* Name() {
      return "Position";
    }

    Position(int x, int y)
        : x_(x), y_(y) {
    }
    operator uint64() const {
      return (static_cast<uint64>(x_) << 32) | y_;
    }
  };

  typedef map <string, Tribe> MapNameTribe;
  typedef std::map<Position, Tribe*> MapPositionTribe;

  MapNameTribe tribes;
  MapPositionTribe tribesPosition;

 public:
  RPCServiceGlobal(const string& instance_name);
  ~RPCServiceGlobal();

 protected:
  void MirrorEmptyType(rpc::CallContext<EmptyType>* call,
                       const EmptyType& e);
  void GetTribeByPosition(rpc::CallContext<Tribe>* call,
                          int32 x, int32 y);
  void AddTribe(rpc::CallContext<bool>* call,
                const Tribe& arg0, int32 x, int32 y);
  void CreateTribe(rpc::CallContext<Tribe>* call,
                   const string& tribename,
                   const vector<Villager>& people,
                   int32 x, int32 y);
  void SetTribeHead(rpc::CallContext<int64>* call,
                    const string& tribename, int64 headid);
  void GetTribeVillager(rpc::CallContext<Villager>* call,
                        const string& tribename, int64 id);
  void Clear(rpc::CallContext<rpc::Void>* call);

 private:
  bool AddTribeInternal(const Tribe& arg0,
                        int32 x, int32 y);
};

#endif  // __NET_RPC_TEST_SAMPLE_SERVER_SERVICES_IMPL_H__
