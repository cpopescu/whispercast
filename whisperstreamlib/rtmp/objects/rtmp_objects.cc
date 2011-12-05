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

#include <whisperlib/common/base/strutil.h>
#include <whisperlib/common/io/num_streaming.h>
#include "rtmp/objects/amf/amf_util.h"
#include "rtmp/objects/amf/amf0_util.h"
#include "rtmp/objects/rtmp_objects.h"

namespace rtmp {

const char* CObject::CoreTypeName(CObject::CoreType type) {
  switch ( type ) {
    CONSIDER(CORE_SKIP);
    CONSIDER(CORE_NULL);
    CONSIDER(CORE_BOOLEAN);
    CONSIDER(CORE_NUMBER);
    CONSIDER(CORE_STRING);
    CONSIDER(CORE_DATE);
    CONSIDER(CORE_ARRAY);
    CONSIDER(CORE_MIXED_MAP);
    CONSIDER(CORE_STRING_MAP);
    CONSIDER(CORE_CLASS_OBJECT);
    CONSIDER(CORE_RECORD_SET);
    CONSIDER(CORE_RECORD_SET_PAGE);
  }
  LOG_FATAL << "Illegal CoreType: " << type;
  return "UNKNOWN";
}

//////////////////////////////////////////////////////////////////////

string CNull::ToString() const {
  return "CNull";
}
AmfUtil::ReadStatus CNull::ReadFromMemoryStream(
  io::MemoryStream* in, AmfUtil::Version version) {
  DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
  if ( in->Size() < sizeof(uint8) )
    return AmfUtil::READ_NO_DATA;
  const Amf0Util::Type obtype =
    Amf0Util::Type(io::NumStreamer::ReadByte(in));
  if ( obtype != Amf0Util::AMF0_TYPE_NULL &&
       obtype != Amf0Util::AMF0_TYPE_UNDEFINED )
    return AmfUtil::READ_CORRUPTED_DATA;
  return AmfUtil::READ_OK;
}
void CNull::WriteToMemoryStream(io::MemoryStream* out,
                                AmfUtil::Version version) const {
  DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
  io::NumStreamer::WriteByte(out, Amf0Util::AMF0_TYPE_NULL);
};

//////////////////////////////////////////////////////////////////////

bool CBoolean::Equals(const CObject& obj) const {
  if ( !CObject::Equals(obj) ) return false;
  const CBoolean& tmp = static_cast<const CBoolean&>(obj);
  return tmp.value() == value_;
}

string CBoolean::ToString() const {
  return strutil::StringPrintf("CBoolean[%s]", value_ ? "true" : "false");
}

AmfUtil::ReadStatus CBoolean::ReadFromMemoryStream(
  io::MemoryStream* in, AmfUtil::Version version) {
  DCHECK_EQ(version, AmfUtil::AMF0_VERSION);

  if ( in->Size() < 2 * sizeof(uint8) )
    return AmfUtil::READ_NO_DATA;
  if ( io::NumStreamer::ReadByte(in) !=
       Amf0Util::AMF0_TYPE_BOOLEAN )
    return AmfUtil::READ_CORRUPTED_DATA;

  const uint8 value = io::NumStreamer::ReadByte(in);
  if ( value == Amf0Util::VALUE_TRUE ) {
    value_ = true;
  } else if ( value == Amf0Util::VALUE_FALSE ) {
    value_ = false;
  } else {
    return AmfUtil::READ_CORRUPTED_DATA;
  }
  return AmfUtil::READ_OK;
}

void CBoolean::WriteToMemoryStream(io::MemoryStream* out,
                                   AmfUtil::Version version) const {
  DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
  io::NumStreamer::WriteByte(out, Amf0Util::AMF0_TYPE_BOOLEAN);
  io::NumStreamer::WriteByte(
    out, value_? Amf0Util::VALUE_TRUE : Amf0Util::VALUE_FALSE);
};

//////////////////////////////////////////////////////////////////////

bool CNumber::Equals(const CObject& obj) const {
  if ( !CObject::Equals(obj) ) return false;
  const CNumber& tmp = static_cast<const CNumber&>(obj);
  return tmp.value() == value_;
}
string CNumber::ToString() const {
  if ( is_int() )
    return strutil::StringPrintf("CNumber(%d)",
                                 static_cast<int>(value_));
  return strutil::StringPrintf("CNumber(%.4f)", value_);
}
AmfUtil::ReadStatus CNumber::ReadFromMemoryStream(
  io::MemoryStream* in, AmfUtil::Version version) {
  DCHECK_EQ(version, AmfUtil::AMF0_VERSION);

  if ( in->Size() < sizeof(uint8) + sizeof(value_) )
    return AmfUtil::READ_NO_DATA;
  if ( io::NumStreamer::ReadByte(in) != Amf0Util::AMF0_TYPE_NUMBER )
    return AmfUtil::READ_CORRUPTED_DATA;

  value_ = io::NumStreamer::ReadDouble(in, common::BIGENDIAN);
  return AmfUtil::READ_OK;
}
void CNumber::WriteToMemoryStream(io::MemoryStream* out,
                                  AmfUtil::Version version) const {
  DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
  io::NumStreamer::WriteByte(out, Amf0Util::AMF0_TYPE_NUMBER);
  io::NumStreamer::WriteDouble(out, value_, common::BIGENDIAN);
}

//////////////////////////////////////////////////////////////////////

bool CString::Equals(const CObject& obj) const {
  if ( !CObject::Equals(obj) ) return false;
  const CString& tmp = static_cast<const CString&>(obj);
  return tmp.value() == value_;
}
string CString::ToString() const {
  return "CString(\"" + value_ + "\")";
}
AmfUtil::ReadStatus CString::ReadFromMemoryStream(
  io::MemoryStream* in, AmfUtil::Version version) {
  DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
  return Amf0Util::ReadString(in, &value_);
}
void CString::WriteToMemoryStream(io::MemoryStream* out,
                                  AmfUtil::Version version) const {
  DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
  Amf0Util::WriteString(out, value_);
}
uint32 CString::EncodingSize(AmfUtil::Version version) const {
  DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
  if ( value_.size() < Amf0Util::LONG_STRING_LENGTH ) {
    return 1 + 2 + value_.size(); // AMF0_TYPE_STRING + uint16 + data
  } else {
    return 1 + 4 + value_.size(); // AMF0_TYPE_LONG_STRING + uint32 + data
  }
}

//////////////////////////////////////////////////////////////////////

bool CDate::Equals(const CObject& obj) const {
  if ( !CObject::Equals(obj) ) return false;
  const CDate& tmp = static_cast<const CDate&>(obj);
  return (timer_ms_ == tmp.timer_ms() &&
          timezone_mins_ == tmp.timezone_mins());
}
string CDate::ToString() const {
  struct tm t;
  time_t tt = timer_ms_ - timezone_mins_ * 60000LL;
  gmtime_r(&tt, &t);
  return strutil::StringPrintf(
      "CDate(%04d-%02d-%02d %02d:%02d:%02d GMT)",
      static_cast<int>(1900 + t.tm_year),
      static_cast<int>(t.tm_mon + 1),
      static_cast<int>(t.tm_mday),
      static_cast<int>(t.tm_hour),
      static_cast<int>(t.tm_min),
      static_cast<int>(t.tm_sec));
}
AmfUtil::ReadStatus CDate::ReadFromMemoryStream(
  io::MemoryStream* in, AmfUtil::Version version) {
  DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
  if ( in->Size() <
       sizeof(uint8) + sizeof(timer_ms_) + sizeof(timezone_mins_) )
    return AmfUtil::READ_NO_DATA;
  if ( io::NumStreamer::ReadByte(in) != Amf0Util::AMF0_TYPE_DATE )
    return AmfUtil::READ_CORRUPTED_DATA;

  timer_ms_ = static_cast<int64>(io::NumStreamer::ReadDouble(in,
      common::BIGENDIAN));
  timezone_mins_ = io::NumStreamer::ReadInt16(in, common::BIGENDIAN);

  return AmfUtil::READ_OK;
}
void CDate::WriteToMemoryStream(io::MemoryStream* out,
                                AmfUtil::Version version) const {
  DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
  io::NumStreamer::WriteByte(out, Amf0Util::AMF0_TYPE_DATE);
  io::NumStreamer::WriteDouble(out, timer_ms_, common::BIGENDIAN);
  io::NumStreamer::WriteInt16(out, timezone_mins_, common::BIGENDIAN);
}

//////////////////////////////////////////////////////////////////////

AmfUtil::ReadStatus CRecordSet::ReadFromMemoryStream(
  io::MemoryStream* in, AmfUtil::Version version) {
  DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
  if ( in->Size() < sizeof(uint8) )
    return AmfUtil::READ_NO_DATA;
  if ( io::NumStreamer::ReadByte(in) !=
       Amf0Util::AMF0_TYPE_CLASS_OBJECT )
    return AmfUtil::READ_CORRUPTED_DATA;
  string tag;
  const AmfUtil::ReadStatus err = Amf0Util::ReadString(in, &tag);
  if ( err != AmfUtil::READ_OK )
    return err;
  if ( tag != "RecordSet" )
    return AmfUtil::READ_CORRUPTED_DATA;
  // Damn - read nothning now ..
  return AmfUtil::READ_OK;
}
void CRecordSet::WriteToMemoryStream(io::MemoryStream* out,
                                     AmfUtil::Version version) const {
  DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
  Amf0Util::WriteClassObjectBegin(out);
  Amf0Util::WriteString(out, "RecordSet");
}

//////////////////////////////////////////////////////////////////////

AmfUtil::ReadStatus CRecordSetPage::ReadFromMemoryStream(
  io::MemoryStream* in, AmfUtil::Version version) {
  DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
  if ( in->Size() < sizeof(uint8) )
    return AmfUtil::READ_NO_DATA;
  if ( io::NumStreamer::ReadByte(in) !=
       Amf0Util::AMF0_TYPE_CLASS_OBJECT )
    return AmfUtil::READ_CORRUPTED_DATA;
  string tag;
  const AmfUtil::ReadStatus err = Amf0Util::ReadString(in, &tag);
  if ( err != AmfUtil::READ_OK )
    return err;
  if ( tag != "RecordSetPage" )
    return AmfUtil::READ_CORRUPTED_DATA;
  // Damn - read nothning now ..
  return AmfUtil::READ_OK;
}

void CRecordSetPage::WriteToMemoryStream(io::MemoryStream* out,
                                         AmfUtil::Version version) const {
  DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
  Amf0Util::WriteClassObjectBegin(out);
  Amf0Util::WriteString(out, "RecordSetPage");
}

//////////////////////////////////////////////////////////////////////

bool CArray::Equals(const CObject& obj) const {
  if ( !CObject::Equals(obj) ) return false;
  const CArray& tmp = static_cast<const CArray&>(obj);
  if ( tmp.data().size() != data_.size() )
    return false;
  for ( int i = 0; i < data_.size(); i++ ) {
    if ( !data_[i]->Equals(*tmp.data()[i]) )
      return false;
  }
  return true;
}
string CArray::ToString() const {
  string s = "CArray( [ ";
  for ( int i = 0; i < data_.size(); i++ ) {
    if ( i > 0 ) s += ", \n";
    s += data_[i]->ToString();
  }
  s += " ]) ";
  return s;
}

AmfUtil::ReadStatus CArray::ReadFromMemoryStream(
  io::MemoryStream* in, AmfUtil::Version version) {
  DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
  Clear();
  if ( in->Size() < sizeof(uint8) + sizeof(int32) )
    return AmfUtil::READ_NO_DATA;
  if ( io::NumStreamer::ReadByte(in) != Amf0Util::AMF0_TYPE_ARRAY )
    return AmfUtil::READ_CORRUPTED_DATA;
  const int32 size = io::NumStreamer::ReadInt32(in, common::BIGENDIAN);
  if ( size > AmfUtil::kMaximumArraySize ) {
    LOG_WARNING << " =======> Structure too long found : " << size;
    return AmfUtil::READ_STRUCT_TOO_LONG;
  }
  CObject* obj;
  for ( int i = 0; i < size; i++ ) {
    const AmfUtil::ReadStatus err = Amf0Util::ReadNextObject(in, &obj);
    if ( err != AmfUtil::READ_OK ) {
      Clear();
      return err;
    }
    data_.push_back(obj);
  }
  return AmfUtil::READ_OK;
}

void CArray::WriteToMemoryStream(io::MemoryStream* out,
                                 AmfUtil::Version version) const {
  DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
  io::NumStreamer::WriteByte(out, Amf0Util::AMF0_TYPE_ARRAY);
  io::NumStreamer::WriteInt32(out, data_.size(), common::BIGENDIAN);
  for ( int i = 0; i < data_.size(); i++ ) {
    data_[i]->WriteToMemoryStream(out, version);
  }
}

void CArray::Clear() {
  for ( Vector::const_iterator it = data_.begin();
        it != data_.end(); ++it ) {
    delete *it;
  }
  data_.clear();
}

//////////////////////////////////////////////////////////////////////

bool CStringMap::Equals(const CObject& obj) const {
  if ( !CObject::Equals(obj) ) return false;
  const CStringMap& tmp = static_cast<const CStringMap&>(obj);
  if ( tmp.data().size() != data_.size() )
    return false;
  for ( Map::const_iterator it = data_.begin(); it != data_.end(); ++it ) {
    const Map::const_iterator it_tmp = tmp.data().find(it->first);
    if ( it_tmp == tmp.data().end() )
      return false;
    if ( !it->second->Equals(*it_tmp->second) )
      return false;
  }
  return true;
}
string CStringMap::ToString() const {
  string s = "CStringMap( { \n";
  for ( list<string>::const_iterator it = keys_.begin();
        it != keys_.end(); ++it ) {
    if ( it != keys_.begin() ) s += ", \n";
    Map::const_iterator itd = data_.find(*it);
    CHECK(itd != data_.end());
    s += "\t\"" + *it + "\": " + itd->second->ToString();
  }
  s += "\n} )";
  return s;
}
AmfUtil::ReadStatus CStringMap::ReadFromMemoryStream(
  io::MemoryStream* in, AmfUtil::Version version) {
  DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
  Clear();

  if ( in->Size() < sizeof(uint8) + sizeof(int16) )
    return AmfUtil::READ_NO_DATA;
  if ( io::NumStreamer::ReadByte(in) != Amf0Util::AMF0_TYPE_OBJECT )
    return AmfUtil::READ_CORRUPTED_DATA;

  AmfUtil::ReadStatus err;
  string key;
  CObject* value = NULL;
  do {
    value = NULL;
    err = Amf0Util::PickString(in, &key);
    if ( err != AmfUtil::READ_OK )
      goto Error;
    if ( !key.empty() ) {
      err = Amf0Util::ReadNextObject(in, &value);
      if ( err != AmfUtil::READ_OK )
        goto Error;
      Set(key, value);
    }
  } while ( !key.empty() );

  return Amf0Util::ReadGenericObjectEnd(in);

  Error:
  delete value;
  Clear();
  return err;
}
void CStringMap::WriteToMemoryStream(io::MemoryStream* out,
                                     AmfUtil::Version version) const {
  DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
  Amf0Util::WriteGenericObjectBegin(out);
  for ( list<string>::const_iterator it = keys_.begin();
        it != keys_.end(); ++it ) {
    Amf0Util::PutString(out, *it);
    Map::const_iterator itd = data_.find(*it);
    CHECK(itd != data_.end());
    itd->second->WriteToMemoryStream(out, version);
  }
  Amf0Util::PutString(out, "");
  Amf0Util::WriteGenericObjectEnd(out);
}
void CStringMap::Clear() {
  for ( Map::const_iterator it = data_.begin(); it != data_.end(); ++it ) {
    delete it->second;
  }
  data_.clear();
  keys_.clear();
}
const CObject* CStringMap::Find(const string& key) const {
  Map::const_iterator it = data_.find(key);
  return it == data_.end() ? NULL : it->second;
}
void CStringMap::Erase(const string& key) {
  Map::iterator it = data_.find(key);
  if ( it == data_.end() ) {
    return;
  }
  delete it->second;
  data_.erase(it);
  for ( list<string>::iterator itk = keys_.begin();
        itk != keys_.end(); ++itk ) {
    if ( *itk == key ) {
      keys_.erase(itk);
      return;
    }
  }
  LOG_FATAL << " out of sync: " << key;
}
void CStringMap::Set(const string& key, CObject* value) {
  Map::iterator it = data_.find(key);
  if ( it == data_.end() ) {
    data_.insert(make_pair(key, value));
    keys_.push_back(key);
  } else {
    delete it->second;
    it->second = value;
  }
}

//////////////////////////////////////////////////////////////////////

const CObject* CMixedMap::Find(const string& key) const {
  Map::const_iterator it = data_.find(key);
  return it == data_.end() ? NULL : it->second;
}
CObject* CMixedMap::Find(const string& key) {
  Map::iterator it = data_.find(key);
  return it == data_.end() ? NULL : it->second;
}
void CMixedMap::Set(const string& key, CObject* value) {
  Map::iterator it = data_.find(key);
  if ( it != data_.end() ) {
    delete it->second;
    it->second = value;
    return;
  }
  data_[key] = value;
}
void CMixedMap::Erase(const string& key) {
  Map::iterator it = data_.find(key);
  if ( it != data_.end() ) {
    delete it->second;
    data_.erase(it);
  }
}
void CMixedMap::SetAll(const CMixedMap& other) {
  for ( Map::const_iterator it = other.data().begin();
        it != other.data().end(); ++it ) {
    Set(it->first, it->second->Clone());
  }
}
void CMixedMap::SetAll(const CStringMap& other) {
  for ( CStringMap::Map::const_iterator it = other.data().begin();
        it != other.data().end(); ++it ) {
    Set(it->first, it->second->Clone());
  }
}
bool CMixedMap::Empty() {
  return data_.empty();
}


void CMixedMap::Clear() {
  for ( Map::const_iterator it = data_.begin(); it != data_.end(); ++it ) {
    delete it->second;
  }
  data_.clear();
}
bool CMixedMap::Equals(const CObject& obj) const {
  if ( !CObject::Equals(obj) ) return false;
  const CMixedMap& tmp = static_cast<const CMixedMap&>(obj);
  if ( tmp.data().size() != data_.size() )
    return false;
  for ( Map::const_iterator it = data_.begin(); it != data_.end(); ++it ) {
    const Map::const_iterator it_tmp = tmp.data().find(it->first);
    if ( it_tmp == tmp.data().end() )
      return false;
    if ( !it->second->Equals(*it_tmp->second) )
      return false;
  }
  return true;
}
string CMixedMap::ToString() const {
  string s = "CMixedMap( { ";
  for ( Map::const_iterator it = data_.begin(); it != data_.end(); ++it ) {
    if ( it != data_.begin() ) s += ", \n";
    s += "\"" + it->first + "\": " + it->second->ToString();
  }
  s += " }) ";
  return s;
}
AmfUtil::ReadStatus CMixedMap::ReadFromMemoryStream(
  io::MemoryStream* in, AmfUtil::Version version) {
  DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
  Clear();

  if ( in->Size() < sizeof(uint8) + sizeof(int32) )
    return AmfUtil::READ_NO_DATA;
  if ( io::NumStreamer::ReadByte(in) !=
       Amf0Util::AMF0_TYPE_MIXED_ARRAY ) {
    LOG_ERROR << " BAD TYPE !! ";
    return AmfUtil::READ_CORRUPTED_DATA;
  }
  // Skip one (it looks) unimportant uint32
  unknown_ = io::NumStreamer::ReadInt32(in, common::BIGENDIAN);

  AmfUtil::ReadStatus err;
  string key;
  CObject* value = NULL;
  do {
    value = NULL;
    err = Amf0Util::PickString(in, &key);
    if ( err != AmfUtil::READ_OK )
      goto Error;
    if ( !key.empty() ) {
      err = Amf0Util::ReadNextObject(in, &value);
      if ( err != AmfUtil::READ_OK )
        goto Error;
      data_.insert(make_pair(key, value));
    }
  } while ( !key.empty() );
  return Amf0Util::ReadGenericObjectEnd(in);

  Error:
  delete value;
  Clear();
  return err;
}
void CMixedMap::WriteToMemoryStream(io::MemoryStream* out,
                                     AmfUtil::Version version) const {
  DCHECK_EQ(version, AmfUtil::AMF0_VERSION);
  io::NumStreamer::WriteByte(out, Amf0Util::AMF0_TYPE_MIXED_ARRAY);
  io::NumStreamer::WriteInt32(out, unknown_, common::BIGENDIAN);
  for ( Map::const_iterator it = data_.begin(); it != data_.end(); ++it ) {
    Amf0Util::PutString(out, it->first);
    it->second->WriteToMemoryStream(out, version);
  }
  Amf0Util::PutString(out, "");
  Amf0Util::WriteGenericObjectEnd(out);
}
}
