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

#include "common/base/log.h"
#include "common/base/errno.h"

#include "services_impl.h"

/********************************************************************/
/*           server user implementation of the services             */
/********************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Service1 service
//
RPCServiceService1::RPCServiceService1(const string& instance_name)
  : ServiceInvokerService1(instance_name) {
}
RPCServiceService1::~RPCServiceService1() {
}
void RPCServiceService1::MirrorVoid(rpc::CallContext<rpc::Void>* call) {
  LOG_WARNING << "RPCServiceService1::MirrorVoid()";
  call->Complete();
}
void RPCServiceService1::MirrorBool(rpc::CallContext<bool>* call,
                                    bool arg) {
  LOG_WARNING << "RPCServiceService1::MirrorBool()";
  call->Complete(arg);
}
void RPCServiceService1::MirrorFloat(rpc::CallContext<double>* call,
                                     double arg) {
  LOG_WARNING << "RPCServiceService1::MirrorFloat()";
  call->Complete(arg);
}
void RPCServiceService1::MirrorInt(rpc::CallContext<int32>* call,
                                   int32 arg) {
  LOG_WARNING << "RPCServiceService1::MirrorInt()";
  call->Complete(arg);
}
void RPCServiceService1::MirrorBigInt(rpc::CallContext<int64>* call,
                                      int64 arg) {
  LOG_WARNING << "RPCServiceService1::MirrorBigInt()";
  call->Complete(arg);
}
void RPCServiceService1::MirrorString(rpc::CallContext<string>* call,
                                      const string& arg) {
  LOG_WARNING << "RPCServiceService1::MirrorString()";
  call->Complete(arg);
}
void RPCServiceService1::MirrorArrayOfInt(
  rpc::CallContext< vector<int32> >* call,
  const vector<int32>& arg) {
  LOG_WARNING << "RPCServiceService1::MirrorArrayOfInt()";
  call->Complete(arg);
}

void RPCServiceService1::MakeArray(
  rpc::CallContext< vector<string> >* call,
  const string& arg1, const string& arg2, const string& arg3) {
  LOG_WARNING << "RPCServiceService1::MakeArray()";
  vector<string> array(3);
  array[0] = arg1;
  array[1] = arg2;
  array[2] = arg3;
  call->Complete(array);
}

void RPCServiceService1::MirrorArrayMapStringArrayInt(
  rpc::CallContext< vector< map< string, vector< int32 > > > >* call,
  const vector< map< string, vector< int32 > > >& arg0) {
  LOG_WARNING << "RPCServiceService1::MirrorArrayMapStringArrayInt()";
  call->Complete(arg0);
}

///////////////////////////////////////////////////////////////////////////////
//
// Service1 service
//
RPCServiceService2::RPCServiceService2(const string& instance_name)
  : ServiceInvokerService2(instance_name) {
}
RPCServiceService2::~RPCServiceService2() {
}
void RPCServiceService2::MirrorA(rpc::CallContext<A>* call,
                                 const A& arg) {
  LOG_WARNING << "RPCServiceService2::MirrorA()";
  call->Complete(arg);
}
void RPCServiceService2::MirrorB(rpc::CallContext<B>* call,
                                 const B& arg) {
  LOG_WARNING << "RPCServiceService2::MirrorB()";
  call->Complete(arg);
}
void RPCServiceService2::MirrorC(rpc::CallContext<C>* call,
                                 const C& arg) {
  LOG_WARNING << "RPCServiceService2::MirrorC()";
  call->Complete(arg);
}
void RPCServiceService2::MirrorD(rpc::CallContext<D>* call,
                                 const D& arg) {
  LOG_WARNING << "RPCServiceService2::MirrorD()";
  // verify D correctness
  //
  const vector<A>& arrayA = arg.a_;
  const vector<B>& arrayB = arg.b_;
  const vector<C>& arrayC = arg.c_;
  CHECK_EQ(arrayA.size(), arrayB.size());
  CHECK_EQ(arrayA.size(), arrayC.size());
  for(unsigned i = 0; i < arrayA.size(); i++)
  {
    const A& a = arrayA[i];
    const B& b = arrayB[i];
    const C& c = arrayC[i];
    CHECK_EQ(a, c.a_.get());
    CHECK_EQ(b, c.b_.get());
  }
  call->Complete(arg);
}
void RPCServiceService2::BuildA(rpc::CallContext<A>* call,
                                const double value) {
  LOG_WARNING << "RPCServiceService2::BuildA()";
  A a;
  a.value_ = value;
  call->Complete(a);
}
void RPCServiceService2::BuildB(rpc::CallContext<B>* call,
                                bool bBool,
                                const vector<string>& names) {
  LOG_WARNING << "RPCServiceService2::BuildB()";
  B b;
  b.bBool_ = bBool;
  b.names_ = names;
  call->Complete(b);
}
void RPCServiceService2::BuildC(rpc::CallContext<C>* call,
                                const A& a, const B& b) {
  LOG_WARNING << "RPCServiceService2::BuildC()";
  C c;
  c.a_ = a;
  c.b_ = b;
  call->Complete(c);
}

void RPCServiceService2::BuildArrayC(
  rpc::CallContext< vector<C> >* call,
  const vector<A>& a, const vector<B>& b)
{
  LOG_WARNING << "RPCServiceService2::BuildArrayC()";
  if ( a.size() != b.size() ) {
    call->Complete(rpc::RPC_GARBAGE_ARGS);
    return;
  }
  const int unsigned size = (unsigned) a.size();

  vector<C> array(size);
  for(unsigned i = 0; i < size; i++) {
    array[i].a_ = a[i];
    array[i].b_ = b[i];
  }
  call->Complete(array);
}

void RPCServiceService2::BuildD(rpc::CallContext<D>* call,
                                const vector<A>& a,
                                const vector<B>& b) {
  LOG_WARNING << "RPCServiceService2::BuildD()";
  CHECK_EQ(a.size(), b.size());

  const int unsigned size = (unsigned) a.size();

  D d;
  d.a_.ref().resize(size);
  d.b_.ref().resize(size);
  d.c_.ref().resize(size);
  for(unsigned i = 0; i < size; i++)
  {
    d.a_.ref()[i] = a[i];
    d.b_.ref()[i] = b[i];
    d.c_.ref()[i].a_ = a[i];
    d.c_.ref()[i].b_ = b[i];
  }
  call->Complete(d);
}
