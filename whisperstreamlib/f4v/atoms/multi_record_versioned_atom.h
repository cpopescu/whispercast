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
#ifndef __MEDIA_F4V_ATOMS_MULTI_RECORD_VERSIONED_ATOM_H__
#define __MEDIA_F4V_ATOMS_MULTI_RECORD_VERSIONED_ATOM_H__

#include <string>
#include <whisperstreamlib/f4v/f4v_encoder.h>
#include <whisperstreamlib/f4v/f4v_decoder.h>
#include <whisperstreamlib/f4v/atoms/versioned_atom.h>

// An atom containing an array of records.
// This is a base class for atoms: CO64, CTTS, STCO, STSC, STSD, STSS, STSZ, STTS.

// defined in f4v_decoder.cc
DECLARE_int32(f4v_moov_record_print_count);

namespace streaming {
namespace f4v {

template <typename RECORD>
class MultiRecordVersionedAtom : public VersionedAtom {
 public:
  typedef RECORD Record;
  typedef vector<RECORD*> RecordVector;
 public:
  MultiRecordVersionedAtom(AtomType type)
    : VersionedAtom(type),
      records_() {
  }
  MultiRecordVersionedAtom(const MultiRecordVersionedAtom<RECORD>& other)
    : VersionedAtom(other),
      records_() {
    for ( typename RecordVector::const_iterator it = other.records().begin();
          it != other.records().end(); ++it ) {
      const RECORD& other_record = **it;
      records_.push_back(other_record.Clone());
    }
  }
  virtual ~MultiRecordVersionedAtom() {
    ClearRecords();
    CHECK(records_.empty());
  }

  const RecordVector& records() const {
    return records_;
  }
  RecordVector& records() {
    return records_;
  }
  void AddRecord(RECORD* record) {
    records_.push_back(record);
  }
  void ClearRecords() {
    for ( typename RecordVector::iterator it = records_.begin();
          it != records_.end(); ++it ) {
      RECORD* record = *it;
      delete record;
    }
    records_.clear();
  }

  /////////////////////////////////////////////////////////////
  // VersionedAtom methods
  //
  // We may override these methods in upper "Atom" class to decode/encode
  // things before or after the regular array of RECORDS.
  //
 protected:
  virtual void GetSubatoms(vector<const BaseAtom*>& subatoms) const {
  }
  virtual BaseAtom* Clone() const = 0;
  virtual TagDecodeStatus DecodeVersionedBody(uint64 size,
                                              io::MemoryStream& in,
                                              Decoder& decoder) {
    if ( in.Size() < 4 ) {
      DATOMLOG << "Not enough data in stream: " << in.Size()
               << " is less than expected: 4";
      return TAG_DECODE_NO_DATA;
    }
    uint32 count = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
    if ( size != 4 + count * RECORD::kEncodingSize ) {
      EATOMLOG << "Wrong atom size, #" << count << " records of "
               << RECORD::kEncodingSize << " bytes each + 4(count field) = "
               << (4 + count * RECORD::kEncodingSize)
               << " , which is different than the atom body size: " << size;
      return TAG_DECODE_ERROR;
    }
    for ( uint32 i = 0; i < count; i++ ) {
      RECORD* record = new RECORD();
      if ( !record->Decode(in) ) {
        EATOMLOG << "Failed to decode record, at index: " << i << "/" << count;
        delete record;
        return TAG_DECODE_ERROR;
      }
      AddRecord(record);
    }
    CHECK_EQ(count, records_.size());
    return TAG_DECODE_SUCCESS;
  }
  virtual void EncodeVersionedBody(io::MemoryStream& out,
                                   Encoder& encoder) const {
    io::NumStreamer::WriteUInt32(&out, records_.size(), common::BIGENDIAN);
    for ( typename RecordVector::const_iterator it = records_.begin();
          it != records_.end(); ++it ) {
      const RECORD* record = *it;
      record->Encode(out);
    }
  }
  virtual uint64 MeasureVersionedBodySize() const {
    // record count field = 4 bytes
    return 4 + records_.size() * RECORD::kEncodingSize;
  }
  virtual string ToStringVersionedBody(uint32 indent) const {
    ostringstream oss;
    oss << "MultiRecord: count: " << records_.size()
        << ", records_: ";
    int32 i = 0;
    for ( typename RecordVector::const_iterator it = records_.begin();
          it != records_.end() && i < FLAGS_f4v_moov_record_print_count;
          ++it, ++i ) {
      const RECORD* record = *it;
      oss << "{" << record->ToString() << "}";
    }
    if ( records_.size() > FLAGS_f4v_moov_record_print_count ) {
      oss << " ... " << (records_.size() - FLAGS_f4v_moov_record_print_count)
          << " records skipped.";
    }
    return oss.str();
  }

 private:
  RecordVector records_;
};

///////////////////////////////////////////////////////////////////////////
// Intended to declare Multi Record atoms like this:
// typedef MultiRecordVersionedAtomWithType<ATOM_CO64, Co64Record> Co64Atom;
// but there seems to be a conflict between class forward declaration
// and typedef:
//
// e.g.
//  class A;
//  class B {
//    ..implementation..
//  };
//  typedef B A;  // error: conflicting declaration ‘typedef class B A’
//                // error: ‘struct A’ has a previous declaration as ‘struct A’
//
//template<AtomType kTYPE, typename RECORD>
//class MultiRecordVersionedAtomWithType
//  : public MultiRecordVersionedAtom<RECORD> {
//public:
// static const AtomType kType = kTYPE;
//
//public:
// MultiRecordVersionedAtomWithType()
//   : MultiRecordVersionedAtom<RECORD>(kType) {
// }
// MultiRecordVersionedAtomWithType(
//     const MultiRecordVersionedAtomWithType<kTYPE, RECORD>& other)
//   : MultiRecordVersionedAtom<RECORD>(other) {
// }
// virtual ~MultiRecordVersionedAtomWithType();
//
// ///////////////////////////////////////////////////////////////////////////
// // Methods from MultiRecordVersionedAtom
// virtual BaseAtom* Clone() const {
//   return new MultiRecordVersionedAtomWithType<kTYPE, RECORD>(*this);
// }
//};

}
}

#endif // __MEDIA_F4V_ATOMS_MULTI_RECORD_VERSIONED_ATOM_H__
