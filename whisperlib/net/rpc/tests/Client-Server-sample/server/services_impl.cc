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

#include "common/base/log.h"
#include "common/base/errno.h"

#include "services_impl.h"

///////////////////////////////////////////////////////////////////////////////
//
// Gigel service
//
RPCServiceGigel::RPCServiceGigel(const string& instance_name)
    : ServiceInvokerGigel(instance_name),
      selector_thread_() {
  selector_thread_.Start();
}
RPCServiceGigel::~RPCServiceGigel() {
}
void RPCServiceGigel::DoSomething(rpc::CallContext<int32>* call,
                                  int32 a, double b) {
  call->Complete(a + int32(b));
}
void RPCServiceGigel::DoSomethingElse(rpc::CallContext<rpc::Void>* call) {
  call->Complete();
}
void RPCServiceGigel::DoDelayReturn(rpc::CallContext<int32>* call,
                                    int32 ms_delay,
                                    int32 return_value) {
  LOG_WARNING << "DoDelayReturn[" << call << "]"
                 " returning " << return_value << " in " << ms_delay << " ms";
  selector_thread_.mutable_selector()->RegisterAlarm(
      NewCallback(this, &RPCServiceGigel::DelayReturn, call, return_value),
      ms_delay);
}

void RPCServiceGigel::DelayReturn(rpc::CallContext<int32>* call,
                                  int32 return_value) {
  LOG_WARNING << "DoDelayReturn[" << call << "]"
                 " returning value " << return_value;
  call->Complete(return_value);
}


//////////////////////////////////////////////////////////////////////
//
// Mitica service
//
RPCServiceMitica::RPCServiceMitica(const string& instance_name)
    : ServiceInvokerMitica(instance_name),
    address_(),
    age_(0),
    person_(),
    family_() {
  // set required fields. Avoid returning a type with unset required fields.
  person_.name_ = "";
  person_.age_ = 0;
  family_.father_.ref().name_ = "";
  family_.father_.ref().age_ = 0;
  family_.mother_.ref().name_ = "";
  family_.mother_.ref().age_ = 0;
}
RPCServiceMitica::~RPCServiceMitica() {
}
void RPCServiceMitica::Initialize(rpc::CallContext<bool>* call,
                                  const string& address,
                                  int32 age) {
  address_ = address;
  age_ = age;
  call->Complete(true);
}
void RPCServiceMitica::Finish(rpc::CallContext<rpc::Void>* call) {
  call->Complete();
}
void RPCServiceMitica::TestMe(rpc::CallContext<int32>* call,
                              int32 a,
                              double b,
                              const string& c) {
  int32 ret = a + int32(b);
  call->Complete(ret);
}
void RPCServiceMitica::Foo(rpc::CallContext<string>* call,
                           bool x) {
  call->Complete(address_);
}
void RPCServiceMitica::SetPerson(rpc::CallContext<rpc::Void>* call,
                                 const Person& p) {
  person_ = p;
  call->Complete();
}
void RPCServiceMitica::GetPerson(rpc::CallContext<Person>* call) {
  call->Complete(person_);
}
void RPCServiceMitica::SetFamily(rpc::CallContext<Family>* call,
                                 const Person& mother,
                                 const Person& father,
                                 const vector<Person>& children) {
  family_.mother_ = mother;
  family_.father_ = father;
  family_.children_ = children;

  call->Complete(family_);
}

void RPCServiceMitica::GetChildren(
    rpc::CallContext<vector<Person> >* call) {
  call->Complete(family_.children_);
}

//////////////////////////////////////////////////////////////////////
//
// Global service
//
RPCServiceGlobal::RPCServiceGlobal(const string& instance_name)
    : ServiceInvokerGlobal(instance_name),
      tribes(),
      tribesPosition() {
}
RPCServiceGlobal::~RPCServiceGlobal() {
}

void RPCServiceGlobal::MirrorEmptyType(rpc::CallContext<EmptyType>* call,
                                       const EmptyType& e) {
  call->Complete(e);
}

void RPCServiceGlobal::GetTribeByPosition(
    rpc::CallContext<Tribe>* call,
    int32 x, int32 y) {
  MapPositionTribe::const_iterator it = tribesPosition.find(Position(x, y));
  if ( it == tribesPosition.end() ) {
    Tribe nullTribe;
    nullTribe.name_ = "";
    nullTribe.people_.ref();
    call->Complete(nullTribe);
    return;
  }
  call->Complete(*(it->second));
}

bool RPCServiceGlobal::AddTribeInternal(
  const Tribe& arg0, int32 x, int32 y) {
  MapPositionTribe::const_iterator pos_it = tribesPosition.find(Position(x, y));
  MapNameTribe::const_iterator name_it = tribes.find(arg0.name_);
  if (pos_it != tribesPosition.end() ||
      name_it != tribes.end()) {
    return false;
  }
  tribes[arg0.name_] = arg0;
  tribesPosition[Position(x, y)] = &tribes[arg0.name_];
  return true;
}

void RPCServiceGlobal::AddTribe(rpc::CallContext<bool>* call,
                                const Tribe& arg0,
                                int32 x, int32 y) {
  call->Complete(AddTribeInternal(arg0, x, y));
}
void RPCServiceGlobal::CreateTribe(rpc::CallContext<Tribe>* call,
                                   const string& tribename,
                                   const vector<Villager>& people,
                                   int32 x, int32 y) {
  Tribe t;
  t.name_ = tribename;
  t.people_.ref(); // make people Available (even if empty)
  for (uint32 i = 0; i < people.size(); i++) {
    const Villager& v = people[i];
    t.people_[v.ID_] = v;
  }

  const bool result = AddTribeInternal(t, x, y);
  if (!result) {
    Tribe nullTribe;
    nullTribe.name_.ref();
    nullTribe.people_.ref();
    call->Complete(nullTribe);
    return;
  }

  call->Complete(t);
}
void RPCServiceGlobal::SetTribeHead(rpc::CallContext<int64>* call,
                                    const string& tribename,
                                    int64 headid) {
  MapNameTribe::iterator it = tribes.find(tribename);
  if ( it == tribes.end() ) {
    call->Complete(-1LL);
    return;
  }
  Tribe& t = it->second;
  if (t.people_.find(headid) == t.people_.end()) {
    call->Complete(-1LL);
    return;
  }

  const int64 prevHeadID = t.head_id_;
  t.head_id_ = headid;
  call->Complete(prevHeadID);
}

void RPCServiceGlobal::GetTribeVillager(rpc::CallContext<Villager>* call,
                                        const string& tribename,
                                        int64 id) {
  MapNameTribe::const_iterator it_tribe = tribes.find(tribename);
  if ( it_tribe != tribes.end() ) {
    const Tribe& t = it_tribe->second;
    const map<int64, Villager>& people = t.people_;
    map<int64, Villager>::const_iterator it_villager = people.find(id);
    if ( it_villager != people.end() ) {
      const Villager& v = it_villager->second;
      call->Complete(v);
      return;
    }
  }

  Villager nullVillager;
  nullVillager.name_ = "";
  nullVillager.ID_ = 0LL;
  call->Complete(nullVillager);
}

void RPCServiceGlobal::Clear(rpc::CallContext<rpc::Void>* call) {
  tribes.clear();
  tribesPosition.clear();
  call->Complete();
}
