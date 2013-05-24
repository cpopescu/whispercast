// (c) 2008 and beyond Emoticon Media
// Author: Cosmin Tudorache

#ifndef SERVICES_IMPL_H_
#define SERVICES_IMPL_H_

#include "invokers.h"

/********************************************************************/
/*             custom implementation of the services                */
/********************************************************************/

class ServiceGigel : public ServiceInvokerGigel
{
public:
  ServiceGigel();
  virtual ~ServiceGigel();

  virtual void DoSomething(rpc::CallContext<rpc::Int> * call, const rpc::Int * a, const rpc::Float* b);
  virtual void DoSomethingElse(rpc::CallContext<rpc::Void> * call);
};

class ServiceMitica : public ServiceInvokerMitica
{
protected:
  // Local (implementation) variables, not defined in services.rpc .
  std::string address_;
  uint8 age_;
  Person person_;
  Family family_;

public:
  ServiceMitica();
  virtual ~ServiceMitica();

  virtual void Initialize(rpc::CallContext<rpc::Bool> * call, const rpc::String* address, const rpc::Int* age);
  virtual void Exit(rpc::CallContext<rpc::Void> * call);
  virtual void TestMe(rpc::CallContext<rpc::Int> * call, const rpc::Int* a, const rpc::Float* b, const rpc::String* c);
  virtual void Foo(rpc::CallContext<rpc::String> * call, const rpc::Date* d);
  virtual void SetPerson(rpc::CallContext<rpc::Void> * call, const Person* arg0);
  virtual void GetPerson(rpc::CallContext<Person> * call);
  virtual void SetFamily(rpc::CallContext<Family> * call, const Person* mother, const Person* father, const rpc::Array< Person >* children);
};

#endif /*SERVICES_IMPL_H_*/
