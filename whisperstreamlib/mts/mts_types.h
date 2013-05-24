// Copyright (c) 2012, Whispersoft s.r.l.
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

#ifndef __MEDIA_MTS_MTS_TYPES_H__
#define __MEDIA_MTS_MTS_TYPES_H__

#include <whisperlib/common/base/ref_counted.h>
#include <whisperstreamlib/mts/mts_consts.h>

namespace streaming {
namespace mts {

enum TSScrambling {
  // NOTE: do NOT modify these values.
  //       They are the same values found in file format! For easy decoding.
  NOT_SCRAMBLED = 0x00,
  SCRAMBLED_RESERVED = 0x01, // Reserved for future use.
  SCRAMBLED_EVEN_KEY = 0x02,
  SCRAMBLED_ODD_KEY = 0x03,
};
const char* TSScramblingName(TSScrambling scrambling);

class TSPacket : public RefCounted {
 public:
  struct AdaptationField {
    DecodeStatus Decode(io::MemoryStream* in) {
      if ( in->Size() < 1 ) { return DECODE_NO_DATA; }
      uint32 size = io::NumStreamer::ReadByte(in);
      if ( in->Size() < size ) { return DECODE_NO_DATA; }
      raw_.AppendStream(in, size);
      return DECODE_SUCCESS;
    }
    uint32 Size() const { return 1 + raw_.Size(); }
    string ToString() const {
      return strutil::StringPrintf("AdaptationField{%d bytes}", raw_.Size());
    }
    // the adaptation field is not decoded, yet
    io::MemoryStream raw_;
  };
  TSPacket()
      : transport_error_indicator_(false),
        payload_unit_start_indicator_(false),
        transport_priority_(false),
        pid_(0),
        scrambling_(NOT_SCRAMBLED),
        continuity_counter_(0),
        adaptation_field_(NULL),
        payload_(NULL) {
  }
  virtual ~TSPacket() {
    delete adaptation_field_;
    adaptation_field_ = NULL;
    delete payload_;
    payload_ = NULL;
  }

  uint32 pid() const { return pid_; }
  const io::MemoryStream* payload() const { return payload_; }

  DecodeStatus Decode(io::MemoryStream* in);
  void Encode(io::MemoryStream* out) const;

  string ToString() const;

 private:
  // Set by demodulator if can't correct errors in the stream, to tell
  // the demultiplexer that the packet has an uncorrectable error.
  // WE're just going to ignore this.
  bool transport_error_indicator_;
  // 1 means start of PES data or PSI otherwise zero only.
  bool payload_unit_start_indicator_;
  // 1 means higher priority than other packets with the same PID.
  bool transport_priority_;
  // packet ID
  uint16 pid_;
  // is the packet scrambled
  TSScrambling scrambling_;

  // Incremented only when a payload is present
  uint8 continuity_counter_;

  AdaptationField* adaptation_field_;

  io::MemoryStream* payload_;
};

////////////////////////////////////////////////////////////////////////

// Program association table
class ProgramAssociationTable : public RefCounted {
 public:
  ProgramAssociationTable()
    : table_id_(0),
      ts_id_(0),
      version_number_(0),
      currently_applicable_(false),
      section_number_(0),
      last_section_number_(0),
      network_pid_(),
      program_map_pid_(),
      crc_(0) {}
  virtual ~ProgramAssociationTable() {}

  DecodeStatus Decode(io::MemoryStream* in);
  void Encode(io::MemoryStream* out) const;

  string ToString() const;
 private:
  uint8 table_id_;
  uint16 ts_id_; // transport stream id
  uint8 version_number_;
  // true: aplicable now
  // false: this is the next table to become valid
  bool currently_applicable_;
  uint8 section_number_;
  uint8 last_section_number_;
  map<uint16, uint16> network_pid_; // program id -> network program id
  map<uint16, uint16> program_map_pid_; // program id -> program map pid
  uint32 crc_;
};

////////////////////////////////////////////////////////////////////////

// Packetized Elementary Stream
class PESPacket : public RefCounted {
 public:
  struct Header {
    // PES scrambling
    TSScrambling scrambling_;
    // flags
    bool is_priority_;
    bool is_data_aligned_;
    bool is_copyright_;
    bool is_original_;
    // Presentation Time Stamp
    uint64 pts_;
    // Decoding Time Stamp
    uint64 dts_;
    // Elementary Stream Clock Reference
    uint64 escr_base_;
    uint64 escr_extension_;
    // Elementary Stream rate
    uint32 es_rate_;
    // fast forward, slow, fast reverse, ...
    uint8 trick_mode_;
    //
    uint8 additional_copy_info_;
    // The CRC for the previous PES packet
    uint16 previous_pes_packet_crc_;

    Header() : scrambling_(NOT_SCRAMBLED),
               is_priority_(false),
               is_data_aligned_(false),
               is_copyright_(false),
               is_original_(false),
               pts_(0),
               dts_(0),
               escr_base_(0),
               escr_extension_(0),
               es_rate_(0),
               trick_mode_(0),
               additional_copy_info_(0),
               previous_pes_packet_crc_(0) {}

    DecodeStatus Decode(io::MemoryStream* in);
    void Encode(io::MemoryStream* out) const;

    string ToString() const;
  };
 public:
  PESPacket() : stream_id_(0), header_(), data_() {}
  virtual ~PESPacket() {}

  DecodeStatus Decode(io::MemoryStream* in);
  void Encode(io::MemoryStream* out) const;

  string ToString() const;

 private:
  uint8 stream_id_;
  Header header_;
  io::MemoryStream data_;
};
class ESPacket {
};

}
}

#endif // __MEDIA_MTS_MTS_ENCODER_H__
