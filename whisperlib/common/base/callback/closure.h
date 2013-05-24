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

#ifndef __COMMON_BASE_CALLBACK_CLOSURE_H__
#define __COMMON_BASE_CALLBACK_CLOSURE_H__

#include <whisperlib/common/base/log.h>

class Closure {
public:
  Closure(bool is_permanent) : is_permanent_(is_permanent) {
    #ifdef _DEBUG
    selector_registered_ = false;
    is_running_ = false;
    #endif
  }
  virtual ~Closure() {
    #ifdef _DEBUG
    CHECK(!selector_registered_) << "Attempting to delete a closure"
                                    " registered in selector";
    CHECK(!is_running_ || is_permanent_) << "Attempting to delete a temporary "
                                            "closure from within RunInternal()";
    #endif
  }
  void Run() {
    const bool permanent = is_permanent();
    #ifdef _DEBUG
    is_running_ = true;
    #endif
    RunInternal();
    if ( !permanent ) {
      #ifdef _DEBUG
      is_running_ = false;
      #endif
      delete this;
    }
  }
  bool is_permanent() const {
    return is_permanent_;
  }
#ifdef _DEBUG
  void set_selector_registered(bool selector_registered) {
    // NOTE: leave these tests with ==, instead of boolean checking
    //       Because when the Closure gets deleted, selector_registered_
    //       becomes a random numeric value. Thus this is a bugtrap for:
    //       selector->RunInSelectLoop(<deleted_closure>);
    if ( selector_registered ) {
      CHECK(selector_registered_ == false);
    } else {
      CHECK(selector_registered_ == true);
    }
    selector_registered_ = selector_registered;
  }
#endif
protected:
  virtual void RunInternal() = 0;
private:
  const bool is_permanent_;
#ifdef _DEBUG
  bool selector_registered_;
  bool is_running_;
#endif
};


//////////////////////////////////////////////////////////////////////
//
// 0 params
//

class Closure0 : public Closure {
public:
  typedef void (*Fun)(void);
  Closure0(bool is_permanent, Fun fun)
    : Closure(is_permanent),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
    (*fun_)();
  }
private:
  Fun fun_;
};

inline Closure0* NewCallback(Closure0::Fun fun) {
  return new Closure0(false, fun);
}
inline Closure0* NewPermanentCallback(Closure0::Fun fun) {
  return new Closure0(true, fun);
}

template<typename C>
class MemberClosure0 : public Closure {
public:
  typedef void (C::*Fun)();
  MemberClosure0(bool is_permanent, C* c, Fun fun)
    : Closure(is_permanent),
    c_(c),

      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
      (c_->*fun_)();
  }
private:
  C* c_;

  Fun fun_;
};
template<typename C>
MemberClosure0<C>* NewCallback(C* c, void (C::*fun)()) {
  return new MemberClosure0<C>(false, c, fun);
}
template<typename C>
MemberClosure0<C>* NewPermanentCallback(C* c, void (C::*fun)()) {
  return new MemberClosure0<C>(true, c, fun);
}

template<typename C>
class ConstMemberClosure0 : public Closure {
public:
  typedef void (C::*Fun)() const;
  ConstMemberClosure0(bool is_permanent, const C* c, Fun fun)
    : Closure(is_permanent),
    c_(c),

      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
      (c_->*fun_)();
  }
private:
  const C* c_;

  Fun fun_;
};
template<typename C>
ConstMemberClosure0<C>* NewCallback(const C* c, void (C::*fun)() const) {
  return new ConstMemberClosure0<C>(false, c, fun);
}
template<typename C>
ConstMemberClosure0<C>* NewPermanentCallback(C* c, void (C::*fun)() const) {
  return new ConstMemberClosure0<C>(true, c, fun);
}


//////////////////////////////////////////////////////////////////////
//
// 1,2,3,4,5,6 - autogenerated with python print_callback.py
//

template<typename T0>
class Closure1 : public Closure {
public:
  typedef void (*Fun)(T0);
  Closure1(bool is_permanent, Fun fun, T0 p0)
    : Closure(is_permanent),
    p0_(p0),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
    (*fun_)(p0_);
  }
private:
  T0 p0_;
  Fun fun_;
};
template<typename T0>
Closure1<T0>* NewCallback(void (*fun)(T0), T0 p0) {
  return new Closure1<T0>(false, fun, p0);
}
template<typename T0>
Closure1<T0>* NewPermanentCallback(void (*fun)(T0), T0 p0) {
  return new Closure1<T0>(true, fun, p0);
}


template<typename C, typename T0>
class MemberClosure1 : public Closure {
public:
  typedef void (C::*Fun)(T0);
  MemberClosure1(bool is_permanent, C* c, Fun fun, T0 p0)
    : Closure(is_permanent),
    c_(c),
    p0_(p0),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
      (c_->*fun_)(p0_);
  }
private:
  C* c_;
  T0 p0_;
  Fun fun_;
};
template<typename C, typename T0>
MemberClosure1<C, T0>* NewCallback(C* c, void (C::*fun)(T0), T0 p0) {
  return new MemberClosure1<C, T0>(false, c, fun, p0);
}
template<typename C, typename T0>
MemberClosure1<C, T0>* NewPermanentCallback(C* c, void (C::*fun)(T0), T0 p0) {
  return new MemberClosure1<C, T0>(true, c, fun, p0);
}


template<typename C, typename T0>
class ConstMemberClosure1 : public Closure {
public:
  typedef void (C::*Fun)(T0) const;
  ConstMemberClosure1(bool is_permanent, const C* c, Fun fun, T0 p0)
    : Closure(is_permanent),
    c_(c),
    p0_(p0),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
      (c_->*fun_)(p0_);
  }
private:
  const C* c_;
  T0 p0_;
  Fun fun_;
};
template<typename C, typename T0>
ConstMemberClosure1<C, T0>* NewCallback(const C* c, void (C::*fun)(T0) const, T0 p0) {
  return new ConstMemberClosure1<C, T0>(false, c, fun, p0);
}
template<typename C, typename T0>
ConstMemberClosure1<C, T0>* NewPermanentCallback(C* c, void (C::*fun)(T0) const, T0 p0) {
  return new ConstMemberClosure1<C, T0>(true, c, fun, p0);
}


template<typename T0, typename T1>
class Closure2 : public Closure {
public:
  typedef void (*Fun)(T0, T1);
  Closure2(bool is_permanent, Fun fun, T0 p0, T1 p1)
    : Closure(is_permanent),
    p0_(p0),
p1_(p1),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
    (*fun_)(p0_, p1_);
  }
private:
  T0 p0_;
T1 p1_;
  Fun fun_;
};
template<typename T0, typename T1>
Closure2<T0, T1>* NewCallback(void (*fun)(T0, T1), T0 p0, T1 p1) {
  return new Closure2<T0, T1>(false, fun, p0, p1);
}
template<typename T0, typename T1>
Closure2<T0, T1>* NewPermanentCallback(void (*fun)(T0, T1), T0 p0, T1 p1) {
  return new Closure2<T0, T1>(true, fun, p0, p1);
}


template<typename C, typename T0, typename T1>
class MemberClosure2 : public Closure {
public:
  typedef void (C::*Fun)(T0, T1);
  MemberClosure2(bool is_permanent, C* c, Fun fun, T0 p0, T1 p1)
    : Closure(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
      (c_->*fun_)(p0_, p1_);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
  Fun fun_;
};
template<typename C, typename T0, typename T1>
MemberClosure2<C, T0, T1>* NewCallback(C* c, void (C::*fun)(T0, T1), T0 p0, T1 p1) {
  return new MemberClosure2<C, T0, T1>(false, c, fun, p0, p1);
}
template<typename C, typename T0, typename T1>
MemberClosure2<C, T0, T1>* NewPermanentCallback(C* c, void (C::*fun)(T0, T1), T0 p0, T1 p1) {
  return new MemberClosure2<C, T0, T1>(true, c, fun, p0, p1);
}


template<typename C, typename T0, typename T1>
class ConstMemberClosure2 : public Closure {
public:
  typedef void (C::*Fun)(T0, T1) const;
  ConstMemberClosure2(bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1)
    : Closure(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
      (c_->*fun_)(p0_, p1_);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
  Fun fun_;
};
template<typename C, typename T0, typename T1>
ConstMemberClosure2<C, T0, T1>* NewCallback(const C* c, void (C::*fun)(T0, T1) const, T0 p0, T1 p1) {
  return new ConstMemberClosure2<C, T0, T1>(false, c, fun, p0, p1);
}
template<typename C, typename T0, typename T1>
ConstMemberClosure2<C, T0, T1>* NewPermanentCallback(C* c, void (C::*fun)(T0, T1) const, T0 p0, T1 p1) {
  return new ConstMemberClosure2<C, T0, T1>(true, c, fun, p0, p1);
}


template<typename T0, typename T1, typename T2>
class Closure3 : public Closure {
public:
  typedef void (*Fun)(T0, T1, T2);
  Closure3(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2)
    : Closure(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
    (*fun_)(p0_, p1_, p2_);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
  Fun fun_;
};
template<typename T0, typename T1, typename T2>
Closure3<T0, T1, T2>* NewCallback(void (*fun)(T0, T1, T2), T0 p0, T1 p1, T2 p2) {
  return new Closure3<T0, T1, T2>(false, fun, p0, p1, p2);
}
template<typename T0, typename T1, typename T2>
Closure3<T0, T1, T2>* NewPermanentCallback(void (*fun)(T0, T1, T2), T0 p0, T1 p1, T2 p2) {
  return new Closure3<T0, T1, T2>(true, fun, p0, p1, p2);
}


template<typename C, typename T0, typename T1, typename T2>
class MemberClosure3 : public Closure {
public:
  typedef void (C::*Fun)(T0, T1, T2);
  MemberClosure3(bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2)
    : Closure(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
      (c_->*fun_)(p0_, p1_, p2_);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
  Fun fun_;
};
template<typename C, typename T0, typename T1, typename T2>
MemberClosure3<C, T0, T1, T2>* NewCallback(C* c, void (C::*fun)(T0, T1, T2), T0 p0, T1 p1, T2 p2) {
  return new MemberClosure3<C, T0, T1, T2>(false, c, fun, p0, p1, p2);
}
template<typename C, typename T0, typename T1, typename T2>
MemberClosure3<C, T0, T1, T2>* NewPermanentCallback(C* c, void (C::*fun)(T0, T1, T2), T0 p0, T1 p1, T2 p2) {
  return new MemberClosure3<C, T0, T1, T2>(true, c, fun, p0, p1, p2);
}


template<typename C, typename T0, typename T1, typename T2>
class ConstMemberClosure3 : public Closure {
public:
  typedef void (C::*Fun)(T0, T1, T2) const;
  ConstMemberClosure3(bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2)
    : Closure(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
      (c_->*fun_)(p0_, p1_, p2_);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
  Fun fun_;
};
template<typename C, typename T0, typename T1, typename T2>
ConstMemberClosure3<C, T0, T1, T2>* NewCallback(const C* c, void (C::*fun)(T0, T1, T2) const, T0 p0, T1 p1, T2 p2) {
  return new ConstMemberClosure3<C, T0, T1, T2>(false, c, fun, p0, p1, p2);
}
template<typename C, typename T0, typename T1, typename T2>
ConstMemberClosure3<C, T0, T1, T2>* NewPermanentCallback(C* c, void (C::*fun)(T0, T1, T2) const, T0 p0, T1 p1, T2 p2) {
  return new ConstMemberClosure3<C, T0, T1, T2>(true, c, fun, p0, p1, p2);
}


template<typename T0, typename T1, typename T2, typename T3>
class Closure4 : public Closure {
public:
  typedef void (*Fun)(T0, T1, T2, T3);
  Closure4(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3)
    : Closure(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
    (*fun_)(p0_, p1_, p2_, p3_);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
  Fun fun_;
};
template<typename T0, typename T1, typename T2, typename T3>
Closure4<T0, T1, T2, T3>* NewCallback(void (*fun)(T0, T1, T2, T3), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new Closure4<T0, T1, T2, T3>(false, fun, p0, p1, p2, p3);
}
template<typename T0, typename T1, typename T2, typename T3>
Closure4<T0, T1, T2, T3>* NewPermanentCallback(void (*fun)(T0, T1, T2, T3), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new Closure4<T0, T1, T2, T3>(true, fun, p0, p1, p2, p3);
}


template<typename C, typename T0, typename T1, typename T2, typename T3>
class MemberClosure4 : public Closure {
public:
  typedef void (C::*Fun)(T0, T1, T2, T3);
  MemberClosure4(bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3)
    : Closure(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
      (c_->*fun_)(p0_, p1_, p2_, p3_);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
  Fun fun_;
};
template<typename C, typename T0, typename T1, typename T2, typename T3>
MemberClosure4<C, T0, T1, T2, T3>* NewCallback(C* c, void (C::*fun)(T0, T1, T2, T3), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new MemberClosure4<C, T0, T1, T2, T3>(false, c, fun, p0, p1, p2, p3);
}
template<typename C, typename T0, typename T1, typename T2, typename T3>
MemberClosure4<C, T0, T1, T2, T3>* NewPermanentCallback(C* c, void (C::*fun)(T0, T1, T2, T3), T0 p0, T1 p1, T2 p2, T3 p3) {
  return new MemberClosure4<C, T0, T1, T2, T3>(true, c, fun, p0, p1, p2, p3);
}


template<typename C, typename T0, typename T1, typename T2, typename T3>
class ConstMemberClosure4 : public Closure {
public:
  typedef void (C::*Fun)(T0, T1, T2, T3) const;
  ConstMemberClosure4(bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3)
    : Closure(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
      (c_->*fun_)(p0_, p1_, p2_, p3_);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
  Fun fun_;
};
template<typename C, typename T0, typename T1, typename T2, typename T3>
ConstMemberClosure4<C, T0, T1, T2, T3>* NewCallback(const C* c, void (C::*fun)(T0, T1, T2, T3) const, T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ConstMemberClosure4<C, T0, T1, T2, T3>(false, c, fun, p0, p1, p2, p3);
}
template<typename C, typename T0, typename T1, typename T2, typename T3>
ConstMemberClosure4<C, T0, T1, T2, T3>* NewPermanentCallback(C* c, void (C::*fun)(T0, T1, T2, T3) const, T0 p0, T1 p1, T2 p2, T3 p3) {
  return new ConstMemberClosure4<C, T0, T1, T2, T3>(true, c, fun, p0, p1, p2, p3);
}


template<typename T0, typename T1, typename T2, typename T3, typename T4>
class Closure5 : public Closure {
public:
  typedef void (*Fun)(T0, T1, T2, T3, T4);
  Closure5(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4)
    : Closure(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
    (*fun_)(p0_, p1_, p2_, p3_, p4_);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
  Fun fun_;
};
template<typename T0, typename T1, typename T2, typename T3, typename T4>
Closure5<T0, T1, T2, T3, T4>* NewCallback(void (*fun)(T0, T1, T2, T3, T4), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new Closure5<T0, T1, T2, T3, T4>(false, fun, p0, p1, p2, p3, p4);
}
template<typename T0, typename T1, typename T2, typename T3, typename T4>
Closure5<T0, T1, T2, T3, T4>* NewPermanentCallback(void (*fun)(T0, T1, T2, T3, T4), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new Closure5<T0, T1, T2, T3, T4>(true, fun, p0, p1, p2, p3, p4);
}


template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4>
class MemberClosure5 : public Closure {
public:
  typedef void (C::*Fun)(T0, T1, T2, T3, T4);
  MemberClosure5(bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4)
    : Closure(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
      (c_->*fun_)(p0_, p1_, p2_, p3_, p4_);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
  Fun fun_;
};
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4>
MemberClosure5<C, T0, T1, T2, T3, T4>* NewCallback(C* c, void (C::*fun)(T0, T1, T2, T3, T4), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new MemberClosure5<C, T0, T1, T2, T3, T4>(false, c, fun, p0, p1, p2, p3, p4);
}
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4>
MemberClosure5<C, T0, T1, T2, T3, T4>* NewPermanentCallback(C* c, void (C::*fun)(T0, T1, T2, T3, T4), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new MemberClosure5<C, T0, T1, T2, T3, T4>(true, c, fun, p0, p1, p2, p3, p4);
}


template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4>
class ConstMemberClosure5 : public Closure {
public:
  typedef void (C::*Fun)(T0, T1, T2, T3, T4) const;
  ConstMemberClosure5(bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4)
    : Closure(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
      (c_->*fun_)(p0_, p1_, p2_, p3_, p4_);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
  Fun fun_;
};
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4>
ConstMemberClosure5<C, T0, T1, T2, T3, T4>* NewCallback(const C* c, void (C::*fun)(T0, T1, T2, T3, T4) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ConstMemberClosure5<C, T0, T1, T2, T3, T4>(false, c, fun, p0, p1, p2, p3, p4);
}
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4>
ConstMemberClosure5<C, T0, T1, T2, T3, T4>* NewPermanentCallback(C* c, void (C::*fun)(T0, T1, T2, T3, T4) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return new ConstMemberClosure5<C, T0, T1, T2, T3, T4>(true, c, fun, p0, p1, p2, p3, p4);
}


template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
class Closure6 : public Closure {
public:
  typedef void (*Fun)(T0, T1, T2, T3, T4, T5);
  Closure6(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
    : Closure(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
    (*fun_)(p0_, p1_, p2_, p3_, p4_, p5_);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
T5 p5_;
  Fun fun_;
};
template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
Closure6<T0, T1, T2, T3, T4, T5>* NewCallback(void (*fun)(T0, T1, T2, T3, T4, T5), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return new Closure6<T0, T1, T2, T3, T4, T5>(false, fun, p0, p1, p2, p3, p4, p5);
}
template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
Closure6<T0, T1, T2, T3, T4, T5>* NewPermanentCallback(void (*fun)(T0, T1, T2, T3, T4, T5), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return new Closure6<T0, T1, T2, T3, T4, T5>(true, fun, p0, p1, p2, p3, p4, p5);
}


template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
class MemberClosure6 : public Closure {
public:
  typedef void (C::*Fun)(T0, T1, T2, T3, T4, T5);
  MemberClosure6(bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
    : Closure(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
      (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
T5 p5_;
  Fun fun_;
};
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
MemberClosure6<C, T0, T1, T2, T3, T4, T5>* NewCallback(C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return new MemberClosure6<C, T0, T1, T2, T3, T4, T5>(false, c, fun, p0, p1, p2, p3, p4, p5);
}
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
MemberClosure6<C, T0, T1, T2, T3, T4, T5>* NewPermanentCallback(C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return new MemberClosure6<C, T0, T1, T2, T3, T4, T5>(true, c, fun, p0, p1, p2, p3, p4, p5);
}


template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
class ConstMemberClosure6 : public Closure {
public:
  typedef void (C::*Fun)(T0, T1, T2, T3, T4, T5) const;
  ConstMemberClosure6(bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
    : Closure(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
      (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
T5 p5_;
  Fun fun_;
};
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
ConstMemberClosure6<C, T0, T1, T2, T3, T4, T5>* NewCallback(const C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return new ConstMemberClosure6<C, T0, T1, T2, T3, T4, T5>(false, c, fun, p0, p1, p2, p3, p4, p5);
}
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
ConstMemberClosure6<C, T0, T1, T2, T3, T4, T5>* NewPermanentCallback(C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return new ConstMemberClosure6<C, T0, T1, T2, T3, T4, T5>(true, c, fun, p0, p1, p2, p3, p4, p5);
}


template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
class Closure7 : public Closure {
public:
  typedef void (*Fun)(T0, T1, T2, T3, T4, T5, T6);
  Closure7(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
    : Closure(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
p6_(p6),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
    (*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
T5 p5_;
T6 p6_;
  Fun fun_;
};
template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
Closure7<T0, T1, T2, T3, T4, T5, T6>* NewCallback(void (*fun)(T0, T1, T2, T3, T4, T5, T6), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
  return new Closure7<T0, T1, T2, T3, T4, T5, T6>(false, fun, p0, p1, p2, p3, p4, p5, p6);
}
template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
Closure7<T0, T1, T2, T3, T4, T5, T6>* NewPermanentCallback(void (*fun)(T0, T1, T2, T3, T4, T5, T6), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
  return new Closure7<T0, T1, T2, T3, T4, T5, T6>(true, fun, p0, p1, p2, p3, p4, p5, p6);
}


template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
class MemberClosure7 : public Closure {
public:
  typedef void (C::*Fun)(T0, T1, T2, T3, T4, T5, T6);
  MemberClosure7(bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
    : Closure(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
p6_(p6),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
      (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
T5 p5_;
T6 p6_;
  Fun fun_;
};
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
MemberClosure7<C, T0, T1, T2, T3, T4, T5, T6>* NewCallback(C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, T6), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
  return new MemberClosure7<C, T0, T1, T2, T3, T4, T5, T6>(false, c, fun, p0, p1, p2, p3, p4, p5, p6);
}
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
MemberClosure7<C, T0, T1, T2, T3, T4, T5, T6>* NewPermanentCallback(C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, T6), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
  return new MemberClosure7<C, T0, T1, T2, T3, T4, T5, T6>(true, c, fun, p0, p1, p2, p3, p4, p5, p6);
}


template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
class ConstMemberClosure7 : public Closure {
public:
  typedef void (C::*Fun)(T0, T1, T2, T3, T4, T5, T6) const;
  ConstMemberClosure7(bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
    : Closure(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
p6_(p6),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
      (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
T5 p5_;
T6 p6_;
  Fun fun_;
};
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
ConstMemberClosure7<C, T0, T1, T2, T3, T4, T5, T6>* NewCallback(const C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, T6) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
  return new ConstMemberClosure7<C, T0, T1, T2, T3, T4, T5, T6>(false, c, fun, p0, p1, p2, p3, p4, p5, p6);
}
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
ConstMemberClosure7<C, T0, T1, T2, T3, T4, T5, T6>* NewPermanentCallback(C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, T6) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
  return new ConstMemberClosure7<C, T0, T1, T2, T3, T4, T5, T6>(true, c, fun, p0, p1, p2, p3, p4, p5, p6);
}


template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
class Closure8 : public Closure {
public:
  typedef void (*Fun)(T0, T1, T2, T3, T4, T5, T6, T7);
  Closure8(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
    : Closure(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
p6_(p6),
p7_(p7),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
    (*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, p7_);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
T5 p5_;
T6 p6_;
T7 p7_;
  Fun fun_;
};
template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
Closure8<T0, T1, T2, T3, T4, T5, T6, T7>* NewCallback(void (*fun)(T0, T1, T2, T3, T4, T5, T6, T7), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
  return new Closure8<T0, T1, T2, T3, T4, T5, T6, T7>(false, fun, p0, p1, p2, p3, p4, p5, p6, p7);
}
template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
Closure8<T0, T1, T2, T3, T4, T5, T6, T7>* NewPermanentCallback(void (*fun)(T0, T1, T2, T3, T4, T5, T6, T7), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
  return new Closure8<T0, T1, T2, T3, T4, T5, T6, T7>(true, fun, p0, p1, p2, p3, p4, p5, p6, p7);
}


template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
class MemberClosure8 : public Closure {
public:
  typedef void (C::*Fun)(T0, T1, T2, T3, T4, T5, T6, T7);
  MemberClosure8(bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
    : Closure(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
p6_(p6),
p7_(p7),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
      (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, p7_);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
T5 p5_;
T6 p6_;
T7 p7_;
  Fun fun_;
};
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
MemberClosure8<C, T0, T1, T2, T3, T4, T5, T6, T7>* NewCallback(C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
  return new MemberClosure8<C, T0, T1, T2, T3, T4, T5, T6, T7>(false, c, fun, p0, p1, p2, p3, p4, p5, p6, p7);
}
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
MemberClosure8<C, T0, T1, T2, T3, T4, T5, T6, T7>* NewPermanentCallback(C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
  return new MemberClosure8<C, T0, T1, T2, T3, T4, T5, T6, T7>(true, c, fun, p0, p1, p2, p3, p4, p5, p6, p7);
}


template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
class ConstMemberClosure8 : public Closure {
public:
  typedef void (C::*Fun)(T0, T1, T2, T3, T4, T5, T6, T7) const;
  ConstMemberClosure8(bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
    : Closure(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
p6_(p6),
p7_(p7),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
      (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, p7_);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
T5 p5_;
T6 p6_;
T7 p7_;
  Fun fun_;
};
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
ConstMemberClosure8<C, T0, T1, T2, T3, T4, T5, T6, T7>* NewCallback(const C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
  return new ConstMemberClosure8<C, T0, T1, T2, T3, T4, T5, T6, T7>(false, c, fun, p0, p1, p2, p3, p4, p5, p6, p7);
}
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
ConstMemberClosure8<C, T0, T1, T2, T3, T4, T5, T6, T7>* NewPermanentCallback(C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
  return new ConstMemberClosure8<C, T0, T1, T2, T3, T4, T5, T6, T7>(true, c, fun, p0, p1, p2, p3, p4, p5, p6, p7);
}


template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
class Closure9 : public Closure {
public:
  typedef void (*Fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8);
  Closure9(bool is_permanent, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
    : Closure(is_permanent),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
p6_(p6),
p7_(p7),
p8_(p8),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
    (*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, p7_, p8_);
  }
private:
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
T5 p5_;
T6 p6_;
T7 p7_;
T8 p8_;
  Fun fun_;
};
template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
Closure9<T0, T1, T2, T3, T4, T5, T6, T7, T8>* NewCallback(void (*fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
  return new Closure9<T0, T1, T2, T3, T4, T5, T6, T7, T8>(false, fun, p0, p1, p2, p3, p4, p5, p6, p7, p8);
}
template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
Closure9<T0, T1, T2, T3, T4, T5, T6, T7, T8>* NewPermanentCallback(void (*fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
  return new Closure9<T0, T1, T2, T3, T4, T5, T6, T7, T8>(true, fun, p0, p1, p2, p3, p4, p5, p6, p7, p8);
}


template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
class MemberClosure9 : public Closure {
public:
  typedef void (C::*Fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8);
  MemberClosure9(bool is_permanent, C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
    : Closure(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
p6_(p6),
p7_(p7),
p8_(p8),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
      (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, p7_, p8_);
  }
private:
  C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
T5 p5_;
T6 p6_;
T7 p7_;
T8 p8_;
  Fun fun_;
};
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
MemberClosure9<C, T0, T1, T2, T3, T4, T5, T6, T7, T8>* NewCallback(C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
  return new MemberClosure9<C, T0, T1, T2, T3, T4, T5, T6, T7, T8>(false, c, fun, p0, p1, p2, p3, p4, p5, p6, p7, p8);
}
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
MemberClosure9<C, T0, T1, T2, T3, T4, T5, T6, T7, T8>* NewPermanentCallback(C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8), T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
  return new MemberClosure9<C, T0, T1, T2, T3, T4, T5, T6, T7, T8>(true, c, fun, p0, p1, p2, p3, p4, p5, p6, p7, p8);
}


template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
class ConstMemberClosure9 : public Closure {
public:
  typedef void (C::*Fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8) const;
  ConstMemberClosure9(bool is_permanent, const C* c, Fun fun, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
    : Closure(is_permanent),
    c_(c),
    p0_(p0),
p1_(p1),
p2_(p2),
p3_(p3),
p4_(p4),
p5_(p5),
p6_(p6),
p7_(p7),
p8_(p8),
      fun_(fun) {
  }
protected:
  virtual void RunInternal() {
      (c_->*fun_)(p0_, p1_, p2_, p3_, p4_, p5_, p6_, p7_, p8_);
  }
private:
  const C* c_;
  T0 p0_;
T1 p1_;
T2 p2_;
T3 p3_;
T4 p4_;
T5 p5_;
T6 p6_;
T7 p7_;
T8 p8_;
  Fun fun_;
};
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
ConstMemberClosure9<C, T0, T1, T2, T3, T4, T5, T6, T7, T8>* NewCallback(const C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
  return new ConstMemberClosure9<C, T0, T1, T2, T3, T4, T5, T6, T7, T8>(false, c, fun, p0, p1, p2, p3, p4, p5, p6, p7, p8);
}
template<typename C, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
ConstMemberClosure9<C, T0, T1, T2, T3, T4, T5, T6, T7, T8>* NewPermanentCallback(C* c, void (C::*fun)(T0, T1, T2, T3, T4, T5, T6, T7, T8) const, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
  return new ConstMemberClosure9<C, T0, T1, T2, T3, T4, T5, T6, T7, T8>(true, c, fun, p0, p1, p2, p3, p4, p5, p6, p7, p8);
}

#endif  // __COMMON_BASE_CALLBACK_CLOSURE_H__
