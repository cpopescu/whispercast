#ifndef WRAPPERS_H_
#define WRAPPERS_H_

#include "net/rpc/lib/client/rpc_service_wrapper.h"
#include "../types.h"

class ServiceWrapperGigel : protected rpc::ServiceWrapper
{
public:
  ServiceWrapperGigel(rpc::IClientConnection & rpcConnection);
  virtual ~ServiceWrapperGigel();

  // public methods of rpc::ServiceWrapper. Deny access to Call & Connect related methods.
  using rpc::ServiceWrapper::SetCallTimeout;
  using rpc::ServiceWrapper::GetCallTimeout;
  using rpc::ServiceWrapper::GetServiceName;
  using rpc::ServiceWrapper::CancelCall;

  /********************************************************/
  /*                    Service methods                   */
  /********************************************************/
  void DoSomething(rpc::CallResult<rpc::Int> & ret, const rpc::Int & a, const rpc::Float & b);
  rpc::CALL_ID DoSomething(Callback1<const rpc::CallResult<rpc::Int> &> * fun, const rpc::Int & a, const rpc::Float & b);

  void DoSomethingElse(rpc::CallResult<rpc::Void> & ret);
  rpc::CALL_ID DoSomethingElse(Callback1<const rpc::CallResult<rpc::Void> &> * fun);
};


class ServiceWrapperMitica : protected rpc::ServiceWrapper
{
public:
  ServiceWrapperMitica(rpc::IClientConnection & rpcConnection);
  virtual ~ServiceWrapperMitica();

  // public methods of rpc::ServiceWrapper. Deny access to Call & Connect related methods.
  using rpc::ServiceWrapper::SetCallTimeout;
  using rpc::ServiceWrapper::GetCallTimeout;
  using rpc::ServiceWrapper::GetServiceName;
  using rpc::ServiceWrapper::CancelCall;

  /********************************************************/
  /*                   Service methods                    */
  /********************************************************/
  void Initialize(rpc::CallResult<rpc::Bool> & ret, const rpc::String & address, const rpc::Int & age);
  rpc::CALL_ID Initialize(Callback1<const rpc::CallResult<rpc::Bool> &> * fun, const rpc::String & address, const rpc::Int & age);

  void Exit(rpc::CallResult<rpc::Void> & ret);
  rpc::CALL_ID Exit(Callback1<const rpc::CallResult<rpc::Void> &> * fun);

  void TestMe(rpc::CallResult<rpc::Int> & ret, const rpc::Int & a, const rpc::Float & b, const rpc::String & c);
  rpc::CALL_ID TestMe(Callback1<const rpc::CallResult<rpc::Int> &> * fun, const rpc::Int & a, const rpc::Float & b, const rpc::String & c);

  void Foo(rpc::CallResult<rpc::String> & ret, const rpc::Date & date);
  rpc::CALL_ID Foo(Callback1<const rpc::CallResult<rpc::String> &> * fun, const rpc::Date & date);

  void SetPerson(rpc::CallResult<rpc::Void> & ret, const Person & p);
  rpc::CALL_ID SetPerson(Callback1<const rpc::CallResult<rpc::Void> &> * fun, const Person & p);

  void GetPerson(rpc::CallResult<Person> & ret);
  rpc::CALL_ID GetPerson(Callback1<const rpc::CallResult<Person> &> * fun);

  void SetFamily(rpc::CallResult<Family> & ret, const Person & mother, const Person & father, const rpc::Array<Person> & children);
  rpc::CALL_ID SetFamily(Callback1<const rpc::CallResult<Family> &> * fun, const Person & mother, const Person & father, const rpc::Array<Person> & children);
};

#endif /*WRAPPERS_H_*/
