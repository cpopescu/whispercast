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

#include <whisperlib/common/io/num_streaming.h>
#include "rtmp/objects/amf/amf0_util.h"
#include "rtmp/objects/rtmp_objects.h"

namespace rtmp {

void Amf0Util::WriteString(io::MemoryStream* out, const char* str) {
  const int32 size = strlen(str);
  if (size < LONG_STRING_LENGTH) {
    io::NumStreamer::WriteByte(out, AMF0_TYPE_STRING);
    io::NumStreamer::WriteInt16(out, int16(size), common::BIGENDIAN);
  } else {
    io::NumStreamer::WriteByte(out, AMF0_TYPE_LONG_STRING);
    io::NumStreamer::WriteInt32(out, size, common::BIGENDIAN);
  }
  out->Write(str, size);
}

void Amf0Util::WriteString(io::MemoryStream* out, const string& str) {
  if (str.size() < LONG_STRING_LENGTH) {
    io::NumStreamer::WriteByte(out, AMF0_TYPE_STRING);
    io::NumStreamer::WriteInt16(out, int16(str.size()), common::BIGENDIAN);
  } else {
    io::NumStreamer::WriteByte(out, AMF0_TYPE_LONG_STRING);
    io::NumStreamer::WriteInt32(out, str.size(), common::BIGENDIAN);
  }
  out->Write(str.data(), str.size());
}

void Amf0Util::WriteClassObjectBegin(io::MemoryStream* out) {
  io::NumStreamer::WriteByte(out, AMF0_TYPE_CLASS_OBJECT);
}

void Amf0Util::WriteGenericObjectBegin(io::MemoryStream* out) {
  io::NumStreamer::WriteByte(out, AMF0_TYPE_OBJECT);
}

void Amf0Util::WriteGenericObjectEnd(io::MemoryStream* out) {
  io::NumStreamer::WriteByte(out, AMF0_TYPE_END_OF_OBJECT);
}

void Amf0Util::PutString(io::MemoryStream* out, const char* str) {
  const uint32 size = strlen(str);
  io::NumStreamer::WriteInt16(out, int16(size), common::BIGENDIAN);
  out->Write(str, size);
}
void Amf0Util::PutString(io::MemoryStream* out, const string& str) {
  io::NumStreamer::WriteInt16(out, int16(str.size()), common::BIGENDIAN);
  out->Write(str.data(), str.size());
}

//////////////////////////////////////////////////////////////////////

AmfUtil::ReadStatus Amf0Util::ReadNextObject(io::MemoryStream* in,
                                             CObject** obj) {
  if ( in->Size() < sizeof(uint8) ) {
    return AmfUtil::READ_NO_DATA;
  }
  // just peek the stream to figure out what Object is next
  const Type obtype = Type(io::NumStreamer::PeekByte(in));
  switch ( obtype ) {
    case AMF0_TYPE_NUMBER:      *obj = new CNumber(); break;
    case AMF0_TYPE_BOOLEAN:     *obj = new CBoolean(); break;
    case AMF0_TYPE_STRING:
    case AMF0_TYPE_LONG_STRING: *obj = new CString(); break;
    case AMF0_TYPE_OBJECT:      *obj = new CStringMap(); break;
    case AMF0_TYPE_NULL:
    case AMF0_TYPE_UNDEFINED:   *obj = new CNull(); break;
    case AMF0_TYPE_MIXED_ARRAY: *obj = new CMixedMap(); break;
    case AMF0_TYPE_ARRAY:       *obj = new CArray(); break;
    case AMF0_TYPE_DATE:        *obj = new CDate(); break;

    case AMF0_TYPE_REFERENCE:
      if ( in->Size() < sizeof(uint16) ) {
        return AmfUtil::READ_NO_DATA;
      }
      LOG_ERROR << "Unsupported reference type found: "
                << io::NumStreamer::ReadInt16(in, common::BIGENDIAN);
      return AmfUtil::READ_UNSUPPORTED_REFERENCES;
    case AMF0_TYPE_CLASS_OBJECT: {
      CString class_name;
      in->MarkerSet();
      in->Skip(1); // the obtype byte which was Peeked
      AmfUtil::ReadStatus err = class_name.Decode(in, AmfUtil::AMF0_VERSION);
      in->MarkerRestore();
      if ( err != AmfUtil::READ_OK ) {
        LOG_ERROR << "Failed to read class_name, err: "
                  << AmfUtil::ReadStatusName(err);
        return err;
      }
      if ( class_name.value() == "RecordSet" ) {
        *obj = new CRecordSet();
      } else if ( class_name.value() == "RecordSetPage" ) {
        *obj = new CRecordSetPage();
      } else {
        LOG_ERROR << "READ_NOT_IMPLEMENTED for AMF0_TYPE_CLASS_OBJECT"
                     " w/ name: [" << class_name << "]";
        return AmfUtil::READ_NOT_IMPLEMENTED;
      }
      break;
    }
    case AMF0_TYPE_XML:
    case AMF0_TYPE_MOVIECLIP:
    case AMF0_TYPE_RECORDSET:
    case AMF0_TYPE_UNSUPPORTED:
    case AMF0_TYPE_AMF3_OBJECT:
      LOG_ERROR << "AMF0 READ_NOT_IMPLEMENTED for " << obtype;
      return AmfUtil::READ_NOT_IMPLEMENTED;

    case AMF0_TYPE_END_OF_OBJECT:
    default:
      LOG_ERROR << "Unknown object type: " << (int)obtype;
      return AmfUtil::READ_CORRUPTED_DATA;
  }
  // the stream is intact, nothing was consumed; decode full object
  DCHECK(*obj != NULL);
  return (*obj)->Decode(in, AmfUtil::AMF0_VERSION);
}


AmfUtil::ReadStatus Amf0Util::ReadString(io::MemoryStream* in, string* out) {
  if ( in->Size() < sizeof(uint8) ) {
    return AmfUtil::READ_NO_DATA;
  }
  const Type obtype = Type(io::NumStreamer::ReadByte(in));
  uint32 len = 0;
  if ( obtype == AMF0_TYPE_STRING ) {
    if ( in->Size() < sizeof(uint16) ) {
      return AmfUtil::READ_NO_DATA;
    }
    len = io::NumStreamer::ReadUInt16(in, common::BIGENDIAN);
  } else if ( obtype == AMF0_TYPE_LONG_STRING ) {
    if ( in->Size() < sizeof(uint32) ) {
      return AmfUtil::READ_NO_DATA;
    }
    len = io::NumStreamer::ReadUInt32(in, common::BIGENDIAN);
  } else {
    LOG_ERROR << "Illegal string type: " << obtype;
    return AmfUtil::READ_CORRUPTED_DATA;
  }
  if ( len > AmfUtil::kMaximumStringReadableData ) {
    LOG_ERROR << "String length: " << len << " too long";
    return AmfUtil::READ_STRUCT_TOO_LONG;
  }
  if ( in->Size() < len ) {
    return AmfUtil::READ_NO_DATA;
  }
  CHECK_EQ(in->ReadString(out, len), len);
  return AmfUtil::READ_OK;
}

AmfUtil::ReadStatus Amf0Util::PickString(io::MemoryStream* in, string* out) {
  if ( in->Size() < sizeof(uint16) ) {
    return AmfUtil::READ_NO_DATA;
  }
  uint32 len = io::NumStreamer::ReadUInt16(in, common::BIGENDIAN);
  if ( in->Size() < len ) {
    return AmfUtil::READ_NO_DATA;
  }
  CHECK_EQ(in->ReadString(out, len), len);
  return AmfUtil::READ_OK;
}

AmfUtil::ReadStatus Amf0Util::ReadGenericObjectEnd(io::MemoryStream* in) {
  if ( in->Size() < sizeof(uint8) ) {
    return AmfUtil::READ_NO_DATA;
  }
  uint8 obtype = io::NumStreamer::ReadByte(in);
  if ( obtype != AMF0_TYPE_END_OF_OBJECT ) {
    LOG_ERROR << "Expected AMF0_TYPE_END_OF_OBJECT, found: "
              << strutil::StringPrintf("%02x", obtype);
    return AmfUtil::READ_CORRUPTED_DATA;
  }
  return AmfUtil::READ_OK;
}
}
