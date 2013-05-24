//
// Automatically generated file
//

#ifndef SERVERWRAPPERS_H_
#define SERVERWRAPPERS_H_

#include "net/rpc/lib/types/rpc_all_types.h"
#include "net/rpc/lib/types/rpc_message.h"
#include "net/rpc/lib/server/rpc_core_types.h"
#include "net/rpc/lib/server/rpc_service_invoker.h"

#include "../types.h" // for custom types

class ServiceInvokerGigel : public rpc::ServiceInvoker
{
public:
  ServiceInvokerGigel();
  virtual ~ServiceInvokerGigel();

  virtual string GetTurntablePage(const string& base_path, const string& url_path) const;
  string GetDoSomethingForm(const string& command_base_path) const;
  string GetDoSomethingElseForm(const string& command_base_path) const;
protected:
  bool Call(rpc::Query * query);

  /*************************************************************************/
  /*         Gigel service methods, to be implemented in subclass         */
  /*************************************************************************/
  virtual void DoSomething(rpc::CallContext<rpc::Int> * call, const rpc::Int * a, const rpc::Float* b) = 0;
  virtual void DoSomethingElse(rpc::CallContext<rpc::Void> * call) = 0;
};

class ServiceInvokerMitica : public rpc::ServiceInvoker
{
public:
  ServiceInvokerMitica();
  virtual ~ServiceInvokerMitica();

  virtual string GetTurntablePage(const string& base_path, const string& url_path) const;
  string GetInitializeForm(const string& command_base_path) const;
  string GetExitForm(const string& command_base_path) const;
  string GetTestMeForm(const string& command_base_path) const;
  string GetFooForm(const string& command_base_path) const;
  string GetSetPersonForm(const string& command_base_path) const;
  string GetGetPersonForm(const string& command_base_path) const;
  string GetSetFamilyForm(const string& command_base_path) const;
  string GetGetChildrenForm(const string& command_base_path) const;
protected:
  bool Call(rpc::Query * q);

  /*************************************************************************/
  /*         Mitica service methods, to be implemented in subclass         */
  /*************************************************************************/
  virtual void Initialize(rpc::CallContext<rpc::Bool> * call, const rpc::String* address, const rpc::Int* age) = 0;
  virtual void Exit(rpc::CallContext<rpc::Void> * call) = 0;
  virtual void TestMe(rpc::CallContext<rpc::Int> * call, const rpc::Int* a, const rpc::Float* b, const rpc::String* c) = 0;
  virtual void Foo(rpc::CallContext<rpc::String> * call, const rpc::Date* d) = 0;
  virtual void SetPerson(rpc::CallContext<rpc::Void> * call, const Person* arg0) = 0;
  virtual void GetPerson(rpc::CallContext<Person> * call) = 0;
  virtual void SetFamily(rpc::CallContext<Family> * call, const Person* mother, const Person* father, const rpc::Array< Person >* children) = 0;
};

#endif /*SERVERWRAPPERS_H_*/
