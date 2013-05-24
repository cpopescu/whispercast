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

#ifndef __NET_RTMP_OBJECTS_AMF_AMF0_UTIL_H__
#define __NET_RTMP_OBJECTS_AMF_AMF0_UTIL_H__

#include <string>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperstreamlib/rtmp/objects/amf/amf_util.h>

namespace rtmp {

class CObject;

//
// Fully static class, with functions to encode rtmp objects into streams
//
class Amf0Util {
 public:
  // Helpers for writing a string
  static void WriteString(io::MemoryStream* out, const char* s);
  static void WriteString(io::MemoryStream* out, const string& s);

  // Writes a string not as an object, but as a simple embeded string
  static void PutString(io::MemoryStream* out, const char* s);
  static void PutString(io::MemoryStream* out, const string& s);

  // Writers for data structures
  static void WriteClassObjectBegin(io::MemoryStream* out);
  static void WriteGenericObjectBegin(io::MemoryStream* out);
  static void WriteGenericObjectEnd(io::MemoryStream* out);

  // Reads and creates the next object in the input stream.
  // Returns NULL in case of not enough data or corrupted stream.
  static AmfUtil::ReadStatus ReadNextObject(io::MemoryStream* in,
                                            CObject** obj);

  // Reads a string object from in (oposes WriteString)
  static AmfUtil::ReadStatus ReadString(io::MemoryStream* in, string* s);

  // Reads a simple sting (opposes PutString);
  static AmfUtil::ReadStatus PickString(io::MemoryStream* in, string* s);

  // Reads and passed over an object end marker
  static AmfUtil::ReadStatus ReadGenericObjectEnd(io::MemoryStream* in);

 public:
  static const int LONG_STRING_LENGTH = 65535;

  enum Type {
    // Number marker constant
    AMF0_TYPE_NUMBER = 0x00,
    // Boolean value marker constant
    AMF0_TYPE_BOOLEAN = 0x01,
    // String marker constant
    AMF0_TYPE_STRING = 0x02,
    // General Object. The object is represented by pairs of
    // {property-string, value-cobject}. Best read in a StringMap.
    AMF0_TYPE_OBJECT = 0x03,
    // Movieclip marker constant
    AMF0_TYPE_MOVIECLIP = 0x04,
    // Null marker constant
    AMF0_TYPE_NULL = 0x05,
    // Undefined marker constant
    AMF0_TYPE_UNDEFINED = 0x06,
    // Object reference marker constant
    AMF0_TYPE_REFERENCE = 0x07,
    // Mixed array marker constant. IntMap
    AMF0_TYPE_MIXED_ARRAY = 0x08,
    // End of object marker constant
    AMF0_TYPE_END_OF_OBJECT = 0x09,
    // Array marker constant
    AMF0_TYPE_ARRAY = 0x0A,
    // Date marker constant
    AMF0_TYPE_DATE = 0x0B,
    // Long string marker constant
    AMF0_TYPE_LONG_STRING = 0x0C,
    // Unsupported type marker constant
    AMF0_TYPE_UNSUPPORTED = 0x0D,
    // Recordset marker constant
    AMF0_TYPE_RECORDSET = 0x0E,
    // XML marker constant
    AMF0_TYPE_XML = 0x0F,
    // Class marker constant. There is a string that shows the class-name,
    // followed by particular data (regarding that class)
    AMF0_TYPE_CLASS_OBJECT = 0x10,
    // Object marker constant (for AMF3)
    AMF0_TYPE_AMF3_OBJECT = 0x11,
  };

  enum {
    VALUE_TRUE  = 0x01,
    VALUE_FALSE = 0x00
  };
};
}

#endif  // __NET_RTMP_OBJECTS_AMF_AMF0_UTIL_H__
