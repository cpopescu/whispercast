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

#ifndef __NET_RPC_LIB_TYPES_RPC_ALL_TYPES_H__
#define __NET_RPC_LIB_TYPES_RPC_ALL_TYPES_H__

#include <string>
#include <map>
#include <vector>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/date.h>
#include <whisperlib/common/base/strutil.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/net/rpc/lib/codec/rpc_decoder.h>

namespace rpc {

#define __NAME_STRINGIZER(x) #x

// Used as an empty class in templated
class Void {
 public:
  Void() { }
  bool operator==(const Void& v) const { return true; }
  bool operator!=(const Void& v) const { return false; }
};

class Encoder;
class Decoder;

class Custom {
 public:
  Custom() {
  }
  virtual ~Custom() {
  }

 protected:
  friend class Encoder;
  friend class Decoder;

  // Write object data using the given encoder.
  // There's no reason for this serialization to fail.
  virtual void SerializeSave(Encoder& enc, io::MemoryStream* out) const = 0;

  //  Read object data from the given decoder.
  virtual DECODE_RESULT SerializeLoad(Decoder& dec, io::MemoryStream& in) = 0;

 public:
  virtual string ToString() const = 0;
  virtual Custom* Clone() const = 0;

 public:
  //////////////////////////////////////////////////////////////////////
  //
  // Attribute type for Custom derived classes.
  //
  // Usage example:
  //       class Person : public Custom {
  //          ...
  //        public:
  //          Attribute<string> name_;
  //          Attribute<vector<string> > children_;
  //          ...
  //       };
  //       Person p;
  //       p.name_ = "Gigel";
  //       vector<string> children;
  //       children.push_back("junior1"); children.push_back("junior2");
  //       p.children_ = children;
  template <typename T>
  class Attribute {
   public:
    Attribute() : value_(), is_set_(false) { }
    Attribute(T& value) : value_(value), is_set_(true) { }
    virtual ~Attribute() { }

    //////////////////////////////////////////////////////////////////////

    operator const T&() const {
      CHECK(is_set_);
      return value_;
    }
    operator T&() {
      is_set_ = true;
      return value_;
    }

    const T& get() const {
      return value_;
    }
    T& ref() {
      is_set_ = true;
      return value_;
    }
    T* ptr() {
      is_set_ = true;
      return &value_;
    }

    //////////////////////////////////////////////////////////////////////

    void set(const T& value) {
      value_ = value;
      is_set_ = true;
    }
    void set(const Attribute<T>& other) {
      if ( other.is_set() ) {
        set(other.get());
      } else {
        reset();
      }
    }
    bool is_set() const {
      return is_set_;
    }
    void reset() {
      is_set_ = false;
    }

    //////////////////////////////////////////////////////////////////////

    const Attribute<T>& operator=(const T& value) {
      this->set(value);
      return *this;
    }
    const Attribute<T>& operator=(const Attribute<T>& other) {
      this->set(other);
      return *this;
    }

    bool operator==(const T& value) const {
      return is_set() && get() == value;
    }
    bool operator==(const Attribute<T>& attr) const {
      return ((!is_set() && !attr.is_set()) ||
              (is_set() && attr.is_set() && get() == attr.get()));
    }
    virtual string ToString() const;

   protected:
    T value_;
    bool is_set_;
  };

  //////////////////////////////////////////////////////////////////////
  //
  // Attribute for which T is an string
  //    - we have this to facilitate access to most popular string members
  //
  class StringAttribute : public Attribute<string> {
   public:
    StringAttribute() : Attribute<string>() { }
    StringAttribute(string& value) : Attribute<string>(value) { }
    virtual ~StringAttribute() { }

    const char* c_str() const {
      return this->get().c_str();
    }
    const char* data() const {
      return this->get().data();
    }
    bool empty() const {
      return this->get().empty();
    }
    size_t size() const {
      return this->get().size();
    }
    size_t length() const {
      return this->get().length();
    }
    const char& operator[] (size_t __pos) const {
      return this->get()[__pos];
    }
    char& operator[] (size_t __pos) {
      return this->ref()[__pos];
    }
    string& append(const string& __str) {
      return this->ref().append(__str);
    }
    string& operator+=(const string& __str) {
      return this->ref().append(__str);
    }
    string& operator+=(const char* __s) {
      return this->ref().append(__s);
    }
    string& operator+=(char __c) {
      this->ref().push_back(__c);
      return this->ref();
    }
    const StringAttribute operator=(const string& value) {
      this->ref() = value;
      return *this;
    }
    const StringAttribute operator=(const char* value) {
      this->ref() = value;
      return *this;
    }
    bool operator==(const string& value) const {
      return this->get() == value;
    }
    bool operator==(const char* value) const {
      return this->get() == value;
    }
    bool operator!=(const string& value) const {
      return this->get() != value;
    }
    bool operator!=(const char* value) const {
      return this->get() != value;
    }
  };

  //////////////////////////////////////////////////////////////////////
  //
  // Attribute for which T is an array
  //    - we have this to facilitate access to most popular vector members
  //
  template <typename T>
  class VectorAttribute : public Attribute<T> {
   public:
    VectorAttribute() : Attribute<T>() { }
    VectorAttribute(T& value) : Attribute<T>(value) { }
    virtual ~VectorAttribute() { }

    // '[]' and 'at'
    typename T::value_type& operator[](
        typename T::size_type __n) {
      return this->ref()[__n];
    }
    const typename T::value_type& operator[](
        typename T::size_type __n) const {
      return this->get()[__n];
    }
    typename T::value_type& at(
        typename T::size_type __n) {
      return this->ref().at(__n);
    }
    const typename T::value_type& at(
        typename T::size_type __n) const {
      return this->get().at(__n);
    }

    // 'empty', 'size', 'capacity', 'clear'
    bool empty() const {
      return this->get().empty();
    }
    typename T::size_type size() const {
      return this->get().size();
    }
    typename T::size_type capacity() const {
      return this->get().capacity();
    }
    void clear() {
      this->ref().clear();
    }

    // 'push_back' and 'pop_back'
    void push_back(
        const typename T::value_type& __x) {
      this->ref().push_back(__x);
    }
    void pop_back() {
      this->ref().pop_back();
    }

    // 'resize' and 'reserve'
    void resize(typename T::size_type __new_size) {
      this->ref().resize(__new_size);
    }
    void reserve(typename T::size_type __n) {
      this->ref().reserve(__n);
    }

    // 'begin' and 'end'
    typename T::iterator begin() {
      return this->ref().begin();
    }
    typename T::iterator end() {
      return this->ref().end();
    }
    typename T::const_iterator& begin() const {
      return this->get().begin();
    }
    typename T::const_iterator& end() const {
      return this->get().end();
    }

    // operator =
    const VectorAttribute<T>& operator=(const T& value) {
      this->set(value);
      return *this;
    }
    const VectorAttribute<T>& operator=(const VectorAttribute<T>& attr) {
      this->set(attr);
      return *this;
    }

  };

  //////////////////////////////////////////////////////////////////////
  //
  // Attribute for which T is an map
  //    - we have this to facilitate access to most popular vector members
  //
  template <typename T>
  class MapAttribute : public Attribute<T> {
   public:
    MapAttribute() : Attribute<T>() { }
    MapAttribute(T& value) : Attribute<T>(value) { }
    virtual ~MapAttribute() { }

    // []  and 'at'
    typename T::mapped_type& operator[](
        const typename T::key_type& __k) {
      return this->ref()[__k];
    }
    const typename T::mapped_type& operator[](
        const typename T::key_type& __k) const {
      return this->get()[__k];
    }
    typename T::mapped_type& at(
        const typename T::key_type& __k) {
      return this->ref().at(__k);
    }
    const typename T::mapped_type& at(
        const typename T::key_type& __k) const {
      return this->get().at(__k);
    }

    // 'insert' and 'erase'
    std::pair<typename T::iterator, bool> insert(
        const typename T::value_type& __x) {
      return this->ref().insert(__x);
    }
    void erase(typename T::key_type __k) {
      this->ref().erase(__k);
    }
    void erase(typename T::iterator __position) {
      this->ref().erase(__position);
    }

    // 'clear', 'empty' and 'size'
    void clear() {
      this->ref().clear();
    }
    bool empty() const {
      return this->get().empty();
    }
    typename T::size_type size() const {
      return this->get().size();
    }

    // 'find'
    typename T::iterator find(
        const typename T::key_type& __x) {
      return this->ref().find(__x);
    }
    typename T::const_iterator find(
        const typename T::key_type& __x) const {
      return this->ref().find(__x);
    }

    // 'begin' and 'end'
    typename T::iterator begin() {
      return this->ref().begin();
    }
    typename T::iterator end() {
      return this->ref().end();
    }
    typename T::const_iterator& begin() const {
      return this->get().begin();
    }
    typename T::const_iterator& end() const {
      return this->get().end();
    }

    // operator =
    const MapAttribute<T>& operator=(const T& value) {
      this->set(value);
      return *this;
    }
    const MapAttribute<T>& operator=(const MapAttribute<T>& attr) {
      this->set(attr);
      return *this;
    }
  };
};

//////////////////////////////////////////////////////////////////////

inline string ToString(bool val) {
  return val ? "true" : "false";
}
inline string ToString(int32 val) {
  return strutil::StringPrintf("%d", int(val));
}
inline string ToString(uint32 val) {
  return strutil::StringPrintf("%u", (unsigned int)(val));
}
inline string ToString(int64 val) {
  return strutil::StringPrintf("%"PRId64"", val);
}
inline string ToString(uint64 val) {
  return strutil::StringPrintf("%"PRIu64"", val);
}
inline string ToString(double val) {
  return strutil::StringPrintf("%g", val);
}
inline string ToString(const string& val) {
  return strutil::StringPrintf("\"%s\"", val.c_str());
}
inline string ToString(const rpc::Void& val) {
  return "VOID";
}
inline string ToString(const rpc::Custom& val) {
  return val.ToString();
}

template <class T>
inline string ToString(const vector<T>& val) {
  return strutil::ToString(val);
}

template <class K, class V>
inline string ToString(const map<K,V>& val) {
  return strutil::ToString(val);
}

template<class T>
inline string ToString(const rpc::Custom::Attribute<T>& val) {
  return rpc::ToString(val.get());
}

template<class T>
inline string rpc::Custom::Attribute<T>::ToString() const {
  if ( is_set() ) {
    return rpc::ToString(value_);
  } else {
    return string("[missing]");
  }
}

}

//////////////////////////////////////////////////////////////////////

inline ostream& operator<<(ostream& os, const rpc::Custom& obj) {
  return os << obj.ToString();
}

inline ostream& operator<<(ostream& os, const rpc::Void& obj) {
  return os << rpc::ToString(obj);
}

template<class K, class V>
inline ostream& operator<<(ostream& os, const map<K, V>& obj) {
  return os << strutil::ToString(obj);
}

template<class T>
inline ostream& operator<<(ostream& os, const vector<T>& obj) {
  return os << strutil::ToString(obj);
}

template <class T>
inline ostream& operator<<(ostream& os,
                           const rpc::Custom::Attribute<T>& attr) {
  return os << attr.ToString();
}

template <class T>
inline ostream& operator<<(ostream& os,
                           const rpc::Custom::StringAttribute& attr) {
  return os << attr.ToString();
}

template <class T>
inline ostream& operator<<(ostream& os,
                           const rpc::Custom::VectorAttribute<T>& attr) {
  return os << attr.ToString();
}

template <class T>
inline ostream& operator<<(ostream& os,
                           const rpc::Custom::MapAttribute<T>& attr) {
  return os << attr.ToString();
}

#endif  // __NET_RPC_LIB_TYPES_RPC_ALL_TYPES_H__
