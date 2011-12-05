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
// Author: Cosmin Tudorache & Catalin Popescu

#ifndef __NET_RTMP_OBJECTS_RTMP_OBJECTS_H__
#define __NET_RTMP_OBJECTS_RTMP_OBJECTS_H__

#include <string>
#include <vector>
#include <list>
#include <map>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/io/buffer/memory_stream.h>

#include <whisperstreamlib/rtmp/objects/amf/amf_util.h>

// Types of data as encoded in rtmp / flv protocols
//
// NOTE: they are **NOT** thread safe !!
//
// Here we define a bunch of types for marshaing rtmp/flv data. The types
// of these are as represented by CoreTypes in CObject::CoreObject.
// We define for each one some marshaling / unmarshaling functions:
//
//  AmfUtil::ReadStatus ReadFromMemoryStream(io::MemoryStream* in,
//                                           AmfUtil::Version version);
//  void WriteToMemoryStream(io::MemoryStream* out,
//                           AmfUtil::Version version) const;
//
// For now we implement only AMF0 marshaling. See ../amf/amf0_util.h
// for a bunch of utility functions / constant names used on the amf0
// stream.
//
// We define here:
//
// CObject:
//  |
//  |--- CNull - more or less a NULL pointer :)
//  |--- CBoolean - boolean  (true/false)
//  |--- CNumber - double (but can be int)
//  |--- CString - well you guess :)
//  |--- CDate - timestamp (double ms based) and timezone offset
//  |
//  |--- CRecordSet - some kind of object - unclear
//  |--- CRecordSetPage - some other xkind of object - unclear
//  |
//  |--- CArray - vector<CObject*>
//  |--- CStringMap - map<string, CObject>
//  |--- CIntMap - map<int, CObject>
//
namespace rtmp {

class CObject {
 public:
  //////////////////////////////////////////////////////////////////////
  //
  // We define here the types of objects that we can have
  //
  enum CoreType {
    CORE_SKIP,   // padding
    // Null type marker
    CORE_NULL,   // no undefined type
    // Boolean type marker
    CORE_BOOLEAN,
    // Number type marker
    CORE_NUMBER,
    // String type marker
    CORE_STRING,
    // Date type marker
    CORE_DATE,

    // Basic stuctures
    // Array type marker
    CORE_ARRAY,
    // Map <cstring, cobject> marker (but w/ a different object id.. seems)
    CORE_MIXED_MAP,
    // Map <cstring,pobject> marker
    CORE_STRING_MAP,

    // XML type marker
    // TODO(cpopescu): NOT Implemented
    // CORE_XML,
    // Object (Hash) type marker.
    // Contains an object of type RecordSet, RecordSetPage, or bean.
    CORE_CLASS_OBJECT,

    // RecordSet type marker
    CORE_RECORD_SET,
    // RecordSetPage type marker
    CORE_RECORD_SET_PAGE,

    // Reference type, this is optional for codecs to support
    // TODO(cpopescu): NOT Implemented
    // OPT_REFERENCE,
  };
  static const char* CoreTypeName(CoreType type);

  // TODO(cpopescu): do we need these ?
  enum {
    // Custom datatype mock mask marker
    CUSTOM_MOCK_MASK = 0x20,
    // Custom datatype AMF mask
    CUSTOM_AMF_MASK = 0x30,
    // Custom datatype RTMP mask
    CUSTOM_RTMP_MASK = 0x40,
    // Custom datatype JSON mask
    CUSTOM_JSON_MASK = 0x50,
    // Custom datatype XML mask
    CUSTOM_XML_MASK = 0x60,
  };

 public:
  //////////////////////////////////////////////////////////////////////
  //
  // The actual class
  //
  CObject(CoreType type)
      : object_type_(type) {
  }
  CObject(const CObject& other)
      : object_type_(other.object_type_) {
  }
  virtual ~CObject() {
  }

  CoreType object_type() const {
    return object_type_;
  }

  // Returns the name of this object type.
  const char* object_type_name() const {
    return CoreTypeName(object_type_);
  }
  // Mostly used for testing..
  virtual bool Equals(const CObject& obj) const {
    return ( obj.object_type() == object_type() );
  }
  // Returns a human readable string
  virtual string ToString() const = 0;
  // Reading / writting
  virtual AmfUtil::ReadStatus ReadFromMemoryStream(
      io::MemoryStream* in, AmfUtil::Version version) = 0;
  virtual void WriteToMemoryStream(
      io::MemoryStream* out, AmfUtil::Version version) const = 0;
  virtual CObject* Clone() const = 0;
  // how many bytes would WriteToMemoryStream() write.
  virtual uint32 EncodingSize(AmfUtil::Version version) const = 0;

 protected:
  const CoreType object_type_;
};

inline ostream& operator<<(ostream& os, const CObject& obj) {
  return os << obj.ToString();
}

//////////////////////////////////////////////////////////////////////

class CNull : public CObject {
 public:
  CNull() : CObject(CObject::CORE_NULL) { }
  CNull(const CNull& other) : CObject(other) { }

  virtual string ToString() const;
  virtual AmfUtil::ReadStatus ReadFromMemoryStream(
      io::MemoryStream* in, AmfUtil::Version version);
  virtual void WriteToMemoryStream(
      io::MemoryStream* out, AmfUtil::Version version) const;
  virtual CObject* Clone() const {
    return new CNull(*this);
  }
  virtual uint32 EncodingSize(AmfUtil::Version version) const {
    DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
    return 1;
  }
};

//////////////////////////////////////////////////////////////////////

class CBoolean : public CObject {
 public:
  CBoolean() : CObject(CObject::CORE_BOOLEAN), value_(false) { }
  CBoolean(bool value) : CObject(CObject::CORE_BOOLEAN), value_(value) { }
  CBoolean(const CBoolean& other) : CObject(other), value_(other.value_) { }
  bool value() const { return value_; }

  virtual bool Equals(const CObject& obj) const;
  virtual string ToString() const;
  virtual AmfUtil::ReadStatus ReadFromMemoryStream(
      io::MemoryStream* in, AmfUtil::Version version);
  virtual void WriteToMemoryStream(
      io::MemoryStream* out, AmfUtil::Version version) const;
  virtual CObject* Clone() const {
    return new CBoolean(*this);
  }
  virtual uint32 EncodingSize(AmfUtil::Version version) const {
    DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
    return 2;
  }

 private:
  bool value_;
};

//////////////////////////////////////////////////////////////////////

class CNumber : public CObject {
 public:
  CNumber() : CObject(CObject::CORE_NUMBER), value_(0) { }
  CNumber(double value) : CObject(CObject::CORE_NUMBER), value_(value) { }
  CNumber(const CNumber& other) : CObject(other), value_(other.value_) { }

  int64 int_value() const { return static_cast<int64>(value_); }
  double value() const { return value_; }
  void set_value(double v) { value_ = v; }
  void set_value(int v) { value_ = v; }

  bool is_int() const { return static_cast<double>(int_value()) == value_; }

  virtual bool Equals(const CObject& obj) const;
  virtual string ToString() const;
  virtual AmfUtil::ReadStatus ReadFromMemoryStream(
      io::MemoryStream* in, AmfUtil::Version version);
  virtual void WriteToMemoryStream(
      io::MemoryStream* out, AmfUtil::Version version) const;
  virtual CObject* Clone() const {
    return new CNumber(*this);
  }
  virtual uint32 EncodingSize(AmfUtil::Version version) const {
    DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
    // AMF0_TYPE_NUMBER + double value
    return 9;
  }

 private:
  double value_;
};

//////////////////////////////////////////////////////////////////////

class CString : public CObject {
 public:
  CString() : CObject(CObject::CORE_STRING) { }
  CString(const string& s) : CObject(CObject::CORE_STRING),  value_(s) { }
  CString(const CString& other) : CObject(other),  value_(other.value_) { }
  const string& value() const { return value_; }
  void set_value(const string& value) { value_ = value; }
  string* mutable_value() { return &value_; }

  virtual bool Equals(const CObject& obj) const;
  virtual string ToString() const;
  virtual AmfUtil::ReadStatus ReadFromMemoryStream(
      io::MemoryStream* in, AmfUtil::Version version);
  virtual void WriteToMemoryStream(
      io::MemoryStream* out, AmfUtil::Version version) const;
  virtual CObject* Clone() const {
    return new CString(*this);
  }
  virtual uint32 EncodingSize(AmfUtil::Version version) const;

 private:
  string value_;
};

//////////////////////////////////////////////////////////////////////

class CDate : public CObject {
 public:
  CDate() : CObject(CObject::CORE_DATE), timer_ms_(0), timezone_mins_(0) {}
  CDate(int64 timer_ms, int16 timezone_mins)
      : CObject(CObject::CORE_DATE),
        timer_ms_(timer_ms),
        timezone_mins_(timezone_mins) {}
  CDate(const CDate& other)
      : CObject(other),
        timer_ms_(other.timer_ms_),
        timezone_mins_(other.timezone_mins_) {}

  int64 timer_ms() const { return timer_ms_; }
  int16 timezone_mins() const { return timezone_mins_; }

  virtual bool Equals(const CObject& obj) const;
  virtual string ToString() const;
  virtual AmfUtil::ReadStatus ReadFromMemoryStream(
      io::MemoryStream* in, AmfUtil::Version version);
  virtual void WriteToMemoryStream(
      io::MemoryStream* out, AmfUtil::Version version) const;
  virtual CObject* Clone() const {
    return new CDate(*this);
  }
  virtual uint32 EncodingSize(AmfUtil::Version version) const {
    DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
    // AMF0_TYPE_DATE + double time + int16 timezone
    return 11;
  }

 private:
  // number of milliseconds since the epoch 1st Jan 1970 UTC
  int64 timer_ms_;
  // should not be used. The player ignores this value.
  int16 timezone_mins_;
};

//////////////////////////////////////////////////////////////////////

// We do not implement these, actually ..
class CRecordSet : public CObject {
 public:
  CRecordSet() : CObject(CObject::CORE_RECORD_SET) { }
  CRecordSet(const CRecordSet& other) : CObject(other) { }
  virtual string ToString() const {
    return "CRecordSet";
  }
  virtual AmfUtil::ReadStatus ReadFromMemoryStream(
      io::MemoryStream* in, AmfUtil::Version version);
  virtual void WriteToMemoryStream(
      io::MemoryStream* out, AmfUtil::Version version) const;
  virtual CObject* Clone() const {
    return new CRecordSet(*this);
  }
  virtual uint32 EncodingSize(AmfUtil::Version version) const {
    DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
    // TODO(cosmin): CRecordSet not fully implemented!!
    //               The '13' is derived from WriteToMemoryStream impl.
    return 13;
  }
};

class CRecordSetPage : public CObject {
 public:
  CRecordSetPage() : CObject(CObject::CORE_RECORD_SET_PAGE) { }
  CRecordSetPage(const CRecordSetPage& other) : CObject(other) { }
  virtual string ToString() const {
    return "CRecordSetPage";
  }
  virtual AmfUtil::ReadStatus ReadFromMemoryStream(
      io::MemoryStream* in, AmfUtil::Version version);
  virtual void WriteToMemoryStream(
      io::MemoryStream* out, AmfUtil::Version version) const;
  virtual CObject* Clone() const {
    return new CRecordSetPage(*this);
  }
  virtual uint32 EncodingSize(AmfUtil::Version version) const {
    DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
    // TODO(cosmin): CRecordSetPage not fully implemented!!
    //               The '17' is derived from WriteToMemoryStream impl.
    return 17;
  }
};

//////////////////////////////////////////////////////////////////////

class CArray : public CObject {
 public:
  typedef vector<CObject*> Vector;
  CArray() : CObject(CObject::CORE_ARRAY) {}
  CArray(int count) : CObject(CObject::CORE_ARRAY), data_(count) {}
  CArray(const CArray& other) : CObject(other), data_(other.data_.size()) {
    for ( uint32 i = 0; i < other.data_.size(); i++ ) {
      data_[i] = other.data_[i]->Clone();
    }
  }
  virtual ~CArray() { Clear(); }

  const Vector& data() const { return data_; }
  Vector& mutable_data() { return data_; }

  void Clear();

  virtual bool Equals(const CObject& obj) const;
  virtual string ToString() const;
  virtual AmfUtil::ReadStatus ReadFromMemoryStream(
      io::MemoryStream* in, AmfUtil::Version version);
  virtual void WriteToMemoryStream(
      io::MemoryStream* out, AmfUtil::Version version) const;
  virtual CObject* Clone() const {
    return new CArray(*this);
  }
  virtual uint32 EncodingSize(AmfUtil::Version version) const {
    DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
    uint32 size = 5; // AMF0_TYPE_ARRAY + int32 size
    for ( uint32 i = 0; i < data_.size(); i++ ) {
      size += data_[i]->EncodingSize(version);
    }
    return size;
  }

 private:
  Vector data_;
};


class CStringMap : public CObject {
 public:
  typedef map<string, CObject*> Map;
  CStringMap() : CObject(CObject::CORE_STRING_MAP), data_(), keys_() { }
  CStringMap(const CStringMap& other)
    : CObject(other),
      data_(),
      keys_(other.keys_) {
    for ( Map::const_iterator it = other.data_.begin();
          it != other.data_.end(); ++it ) {
      const string& key = it->first;
      const CObject* value = it->second;
      data_[key] = value->Clone();
    }
  }
  virtual ~CStringMap() { Clear(); }

  const Map& data() const { return data_; }
  // finder
  const CObject* Find(const string& key) const;
  // insert/update key
  void Set(const string& key, CObject* value);
  // erase key
  void Erase(const string& key);

  void Clear();

  virtual bool Equals(const CObject& obj) const;
  virtual string ToString() const;
  virtual AmfUtil::ReadStatus ReadFromMemoryStream(
      io::MemoryStream* in, AmfUtil::Version version);
  virtual void WriteToMemoryStream(
      io::MemoryStream* out, AmfUtil::Version version) const;
  virtual CObject* Clone() const {
    return new CStringMap(*this);
  }
  virtual uint32 EncodingSize(AmfUtil::Version version) const {
    DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
    uint32 size = 1; // AMF0_TYPE_OBJECT
    for ( Map::const_iterator it = data_.begin(); it != data_.end(); ++it ) {
      size += 2 + it->first.size() +             // putString(key)
              it->second->EncodingSize(version); // encode(value)
    }
    size += 2; // putString ""
    size += 1; // AMF0_TYPE_END_OF_OBJECT
    return size;
  }

 private:
  // NOTE: when we implemented this w/ hash_map we used to initialize
  //       buckets to this. If switching back, re-enable this.
  // static const int kDefaultSize = 5;
  Map data_;
  list<string> keys_;
};

class CMixedMap : public CObject {
 public:
  typedef map<string, CObject*> Map;
  CMixedMap()
      : CObject(CObject::CORE_MIXED_MAP),
        unknown_(0) { }
  CMixedMap(const CMixedMap& other)
      : CObject(other),
        unknown_(other.unknown_),
        data_() {
    for ( Map::const_iterator it = other.data_.begin();
          it != other.data_.end(); ++it ) {
      const string& key = it->first;
      const CObject* value = it->second;
      data_[key] = value->Clone();
    }
  }
  virtual ~CMixedMap() { Clear(); }

  const Map& data() const { return data_; }
  //Map& mutable_data() { return data_; }

  // finder
  const CObject* Find(const string& key) const;
  CObject* Find(const string& key);
  // insert/update key
  void Set(const string& key, CObject* value);
  // erase key
  void Erase(const string& key);
  // insert/update all the items in 'other' map
  void SetAll(const CMixedMap& other);
  void SetAll(const CStringMap& other);
  // test empty
  bool Empty();

  void Clear();

  virtual bool Equals(const CObject& obj) const;
  virtual string ToString() const;
  virtual AmfUtil::ReadStatus ReadFromMemoryStream(
      io::MemoryStream* in, AmfUtil::Version version);
  virtual void WriteToMemoryStream(
      io::MemoryStream* out, AmfUtil::Version version) const;
  virtual CObject* Clone() const {
    return new CMixedMap(*this);
  }
  virtual uint32 EncodingSize(AmfUtil::Version version) const {
    DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
    uint32 size = 1; // AMF0_TYPE_MIXED_ARRAY
    size += 4; // int32 unknown_
    for ( Map::const_iterator it = data_.begin(); it != data_.end(); ++it ) {
      size += 2 + it->first.size() +             // putString(key)
              it->second->EncodingSize(version); // encode(value)
    }
    size += 2; // putString ""
    size += 1; // AMF0_TYPE_END_OF_OBJECT
    return size;
  }


 private:
  // NOTE: when we implemented this w/ hash_map we used to initialize
  //       buckets to this. If switching back, re-enable this.
  // static const int kDefaultSize = 5;
  int32 unknown_;
  Map data_;
};
//////////////////////////////////////////////////////////////////////
}

#endif  // __NET_RTMP_OBJECTS_RTMP_OBJECTS_H__
