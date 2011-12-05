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

#ifndef SERVICES_IMPL_H_
#define SERVICES_IMPL_H_

#include "auto/types.h"
#include "auto/invokers.h"

class RPCServiceService1 : public ServiceInvokerService1 {
public:
  RPCServiceService1(const string& instance_name);
  virtual ~RPCServiceService1();

protected:
  void MirrorVoid(rpc::CallContext<rpc::Void>* call);
  void MirrorBool(rpc::CallContext<bool>* call, bool arg0);
  void MirrorFloat(rpc::CallContext<double>* call, double arg0);
  void MirrorInt(rpc::CallContext<int32>* call, int32 arg0);
  void MirrorBigInt(rpc::CallContext<int64>* call, int64 arg0);
  void MirrorString(rpc::CallContext<string>* call, const string& arg0);
  void MirrorArrayOfInt(rpc::CallContext< vector<int32> >* call,
                        const vector< int32 >& arg0);
  void MakeArray(rpc::CallContext< vector<string> >* call,
                 const string& sz1,
                 const string& sz2,
                 const string& sz3);
  void MirrorArrayMapStringArrayInt(
      rpc::CallContext<vector<map<string, vector<int32 > > > >* call,
      const vector<map<string, vector<int32 > > >& arg0);
};

class RPCServiceService2 : public ServiceInvokerService2 {
public:
  RPCServiceService2(const string& instance_name);
  virtual ~RPCServiceService2();

protected:
  void MirrorA(rpc::CallContext<A>* call, const A& arg0);
  void MirrorB(rpc::CallContext<B>* call, const B& arg0);
  void MirrorC(rpc::CallContext<C>* call, const C& arg0);
  void MirrorD(rpc::CallContext<D>* call, const D& arg0);
  void BuildA(rpc::CallContext<A>* call, double arg0);
  void BuildB(rpc::CallContext<B>* call,
              bool arg0, const vector< string >& arg1);
  void BuildC(rpc::CallContext<C>* call, const A& a, const B& b);
  void BuildArrayC(rpc::CallContext< vector<C> >* call,
                   const vector< A >& a, const vector< B >& b);
  void BuildD(rpc::CallContext<D>* call,
              const vector< A >& a, const vector< B >& b);
};

#endif  // SERVICES_IMPL_H_
