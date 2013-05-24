// (c) 2008 and beyond Emoticon Media
// Author: Cosmin Tudorache

#include "common/base/types.h"
#include "common/base/log.h"
#include "common/base/errno.h"

#include "services_impl.h"

/********************************************************************/
/*             custom implementation of the services                */
/********************************************************************/

ServiceGigel::ServiceGigel() : ServiceInvokerGigel() {}
ServiceGigel::~ServiceGigel() {}

void ServiceGigel::DoSomething(rpc::CallContext<rpc::Int>* call, const rpc::Int32 * a, const rpc::Float * b) {
  LOG_INFO << "Gigel::DoSomething(" << *a << "," << *b << ")";
  rpc::Int32 ret = *a + (int)*b;
  LOG_INFO << "Gigel::DoSomething returning => " << ret;
  call->Complete(ret);
}
void ServiceGigel::DoSomethingElse(rpc::CallContext<rpc::Void>* call) {
  LOG_INFO << "Gigel::DoSomethingElse()";
  rpc::Void ret;
  LOG_INFO << "Gigel::DoSomethingElse returning => " << ret;
  call->Complete(ret);
}

ServiceMitica::ServiceMitica()
  : ServiceInvokerMitica(),
    address_(),
    age_(0),
    person_(),
    family_() {
}
ServiceMitica::~ServiceMitica() {}

void ServiceMitica::Initialize(rpc::CallContext<rpc::Bool>* call, const rpc::String * address, const rpc::Int * age) {
  LOG_INFO << "Mitica::Initialize(" << *address << "," << *age << ")";
  address_ = address->CStr();
  age_ = *age;
  rpc::Bool ret(true);
  LOG_INFO << "Mitica::Initialize returning => " << ret;
  call->Complete(ret);
}
void ServiceMitica::Exit(rpc::CallContext<rpc::Void>* call) {
  LOG_INFO << "Mitica::Exit()";
  rpc::Void ret;
  LOG_INFO << "Mitica::Exit returning => " << ret;
  call->Complete(ret);
}
void ServiceMitica::TestMe(rpc::CallContext<rpc::Int>* call, const rpc::Int32 * a, const rpc::Float * b, const rpc::String * c) {
  LOG_INFO << "Mitica::TestMe(" << *a << "," << *b << "," << *c << ")";
  rpc::Int32 ret = ((int)*a + (int)(float)*b);
  LOG_INFO << "Mitica::TestMe returning => " << ret;
  call->Complete(ret);
}
void ServiceMitica::Foo(rpc::CallContext<rpc::String>* call, const rpc::Date * date) {
  LOG_INFO << "Mitica::Foo(" << *date << ")";
  rpc::String ret(address_);
  LOG_INFO << "Mitica::Foo returning => " << ret;
  call->Complete(ret);
}
void ServiceMitica::SetPerson( rpc::CallContext<rpc::Void>* call, const Person * p) {
  LOG_INFO << "Mitica::SetPerson(" << *p << ")";
  person_ = *p;
  rpc::Void ret;
  LOG_INFO << "Mitica::SetPerson returning => " << ret;
  call->Complete(ret);
}
void ServiceMitica::GetPerson(rpc::CallContext<Person>* call) {
  LOG_INFO << "Mitica::GetPerson()";
  LOG_INFO << "Mitica::GetPerson returning => " << person_;
  call->Complete(person_);
}

void ServiceMitica::SetFamily(
  rpc::CallContext<Family>* call,
  const Person* mother,
  const Person* father,
  const rpc::Array<Person>* children) {
  LOG_INFO << "Mitica::SetFamily(" << *mother << ", "
                                   << *father << ", "
                                   << *children << ")"
                                  ;
  family_.mother_.Set(*mother);
  family_.father_.Set(*father);
  family_.children_.Set(*children);

  LOG_INFO << "Mitica::SetFamily returning => " << family_;
  call->Complete(family_);
}
