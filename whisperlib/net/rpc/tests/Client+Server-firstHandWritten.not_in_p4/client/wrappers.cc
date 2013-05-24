
#include <memory>
#include "wrappers.h"

/**************************************************************************/
/*                        ServiceWrapperGigel                         */
/**************************************************************************/
ServiceWrapperGigel::ServiceWrapperGigel(rpc::IClientConnection & rpcConnection)
  : rpc::ServiceWrapper(rpcConnection, "Gigel")
{
}
ServiceWrapperGigel::~ServiceWrapperGigel()
{
}

void ServiceWrapperGigel::DoSomething(rpc::CallResult<rpc::Int> & ret, const rpc::Int & a, const rpc::Float & b)
{
  io::IOMemoryStream params;
  std::auto_ptr<rpc::Encoder> encoder(GetCodec().CreateEncoder(params));
  encoder->EncodeArrayStart(2);
  encoder->Encode(a);
  encoder->EncodeArrayContinue();
  encoder->Encode(b);
  encoder->EncodeArrayEnd();
  Call("DoSomething", params, ret);
}
rpc::CALL_ID ServiceWrapperGigel::DoSomething(Callback1<const rpc::CallResult<rpc::Int> &> * fun, const rpc::Int & a, const rpc::Float & b)
{
  io::IOMemoryStream params;
  std::auto_ptr<rpc::Encoder> encoder(GetCodec().CreateEncoder(params));
  encoder->EncodeArrayStart(2);
  encoder->Encode(a);
  encoder->EncodeArrayContinue();
  encoder->Encode(b);
  encoder->EncodeArrayEnd();
  return AsyncCall<rpc::Int>("DoSomething", params, fun);
}

void ServiceWrapperGigel::DoSomethingElse(rpc::CallResult<rpc::Void> & ret)
{
  io::IOMemoryStream params;
  std::auto_ptr<rpc::Encoder> encoder(GetCodec().CreateEncoder(params));
  encoder->EncodeArrayStart(0);
  encoder->EncodeArrayEnd();
  Call("DoSomethingElse", params, ret);
}
rpc::CALL_ID ServiceWrapperGigel::DoSomethingElse(Callback1<const rpc::CallResult<rpc::Void> &> * fun)
{
  io::IOMemoryStream params;
  std::auto_ptr<rpc::Encoder> encoder(GetCodec().CreateEncoder(params));
  encoder->EncodeArrayStart(0);
  encoder->EncodeArrayEnd();
  return AsyncCall<rpc::Void>("DoSomethingElse", params, fun);
}

/**************************************************************************/
/*                        ServiceWrapperMitica                         */
/**************************************************************************/
ServiceWrapperMitica::ServiceWrapperMitica(rpc::IClientConnection & rpcConnection)
  : rpc::ServiceWrapper(rpcConnection, "Mitica")
{
}
ServiceWrapperMitica::~ServiceWrapperMitica()
{
}

void ServiceWrapperMitica::Initialize(rpc::CallResult<rpc::Bool> & ret, const rpc::String & address, const rpc::Int & age)
{
  io::IOMemoryStream params;
  std::auto_ptr<rpc::Encoder> encoder(GetCodec().CreateEncoder(params));
  encoder->EncodeArrayStart(2);
  encoder->Encode(address);
  encoder->EncodeArrayContinue();
  encoder->Encode(age);
  encoder->EncodeArrayEnd();
  Call("Initialize", params, ret);
}
rpc::CALL_ID ServiceWrapperMitica::Initialize(Callback1<const rpc::CallResult<rpc::Bool> &> * fun, const rpc::String & address, const rpc::Int & age)
{
  io::IOMemoryStream params;
  std::auto_ptr<rpc::Encoder> encoder(GetCodec().CreateEncoder(params));
  encoder->EncodeArrayStart(2);
  encoder->Encode(address);
  encoder->EncodeArrayContinue();
  encoder->Encode(age);
  encoder->EncodeArrayEnd();
  return AsyncCall("Initialize", params, fun);
}

void ServiceWrapperMitica::Exit(rpc::CallResult<rpc::Void> & ret)
{
  io::IOMemoryStream params;
  std::auto_ptr<rpc::Encoder> encoder(GetCodec().CreateEncoder(params));
  encoder->EncodeArrayStart(0);
  encoder->EncodeArrayEnd();
  Call("Exit", params, ret);
}
rpc::CALL_ID ServiceWrapperMitica::Exit(Callback1<const rpc::CallResult<rpc::Void> &> * fun)
{
  io::IOMemoryStream params;
  std::auto_ptr<rpc::Encoder> encoder(GetCodec().CreateEncoder(params));
  encoder->EncodeArrayStart(0);
  encoder->EncodeArrayEnd();
  return AsyncCall("Exit", params, fun);
}

void ServiceWrapperMitica::TestMe(rpc::CallResult<rpc::Int> & ret, const rpc::Int & a, const rpc::Float & b, const rpc::String & c)
{
  io::IOMemoryStream params;
  std::auto_ptr<rpc::Encoder> encoder(GetCodec().CreateEncoder(params));
  encoder->EncodeArrayStart(3);
  encoder->Encode(a);
  encoder->EncodeArrayContinue();
  encoder->Encode(b);
  encoder->EncodeArrayContinue();
  encoder->Encode(c);
  encoder->EncodeArrayEnd();
  Call("TestMe", params, ret);
}
rpc::CALL_ID ServiceWrapperMitica::TestMe(Callback1<const rpc::CallResult<rpc::Int> &> * fun, const rpc::Int & a, const rpc::Float & b, const rpc::String & c)
{
  io::IOMemoryStream params;
  std::auto_ptr<rpc::Encoder> encoder(GetCodec().CreateEncoder(params));
  encoder->EncodeArrayStart(3);
  encoder->Encode(a);
  encoder->EncodeArrayContinue();
  encoder->Encode(b);
  encoder->EncodeArrayContinue();
  encoder->Encode(c);
  encoder->EncodeArrayEnd();
  return AsyncCall("TestMe", params, fun);
}

void ServiceWrapperMitica::Foo(rpc::CallResult<rpc::String> & ret, const rpc::Date & date)
{
  io::IOMemoryStream params;
  std::auto_ptr<rpc::Encoder> encoder(GetCodec().CreateEncoder(params));
  encoder->EncodeArrayStart(1);
  encoder->Encode(date);
  encoder->EncodeArrayEnd();
  Call("Foo", params, ret);
}
rpc::CALL_ID ServiceWrapperMitica::Foo(Callback1<const rpc::CallResult<rpc::String> &> * fun, const rpc::Date & date)
{
  io::IOMemoryStream params;
  std::auto_ptr<rpc::Encoder> encoder(GetCodec().CreateEncoder(params));
  encoder->EncodeArrayStart(1);
  encoder->Encode(date);
  encoder->EncodeArrayEnd();
  return AsyncCall("Foo", params, fun);
}

void ServiceWrapperMitica::SetPerson(rpc::CallResult<rpc::Void> & ret, const Person & p)
{
  io::IOMemoryStream params;
  std::auto_ptr<rpc::Encoder> encoder(GetCodec().CreateEncoder(params));
  encoder->EncodeArrayStart(1);
  encoder->Encode(p);
  encoder->EncodeArrayEnd();
  Call("SetPerson", params, ret);
}
rpc::CALL_ID ServiceWrapperMitica::SetPerson(Callback1<const rpc::CallResult<rpc::Void> &> * fun, const Person & p)
{
  io::IOMemoryStream params;
  std::auto_ptr<rpc::Encoder> encoder(GetCodec().CreateEncoder(params));
  encoder->EncodeArrayStart(1);
  encoder->Encode(p);
  encoder->EncodeArrayEnd();
  return AsyncCall("SetPerson", params, fun);
}

void ServiceWrapperMitica::GetPerson(rpc::CallResult<Person> & ret)
{
  io::IOMemoryStream params;
  std::auto_ptr<rpc::Encoder> encoder(GetCodec().CreateEncoder(params));
  encoder->EncodeArrayStart(0);
  encoder->EncodeArrayEnd();
  Call("GetPerson", params, ret);
}
rpc::CALL_ID ServiceWrapperMitica::GetPerson(Callback1<const rpc::CallResult<Person> &> * fun)
{
  io::IOMemoryStream params;
  std::auto_ptr<rpc::Encoder> encoder(GetCodec().CreateEncoder(params));
  encoder->EncodeArrayStart(0);
  encoder->EncodeArrayEnd();
  return AsyncCall("GetPerson", params, fun);
}

void ServiceWrapperMitica::SetFamily(rpc::CallResult<Family> & ret, const Person & mother, const Person & father, const rpc::Array<Person> & children)
{
  io::IOMemoryStream params;
  std::auto_ptr<rpc::Encoder> encoder(GetCodec().CreateEncoder(params));
  encoder->EncodeArrayStart(3);
  encoder->Encode(mother);
  encoder->EncodeArrayContinue();
  encoder->Encode(father);
  encoder->EncodeArrayContinue();
  encoder->Encode(children);
  encoder->EncodeArrayEnd();
  Call("SetFamily", params, ret);
}
rpc::CALL_ID ServiceWrapperMitica::SetFamily(Callback1<const rpc::CallResult<Family> &> * fun, const Person & mother, const Person & father, const rpc::Array<Person> & children)
{
  io::IOMemoryStream params;
  std::auto_ptr<rpc::Encoder> encoder(GetCodec().CreateEncoder(params));
  encoder->EncodeArrayStart(3);
  encoder->Encode(mother);
  encoder->EncodeArrayContinue();
  encoder->Encode(father);
  encoder->EncodeArrayContinue();
  encoder->Encode(children);
  encoder->EncodeArrayEnd();
  return AsyncCall("SetFamily", params, fun);
}
