#include "f4v/atoms/movie/esds_atom.h"
#include <whisperlib/common/io/num_streaming.h>
#include <whisperlib/common/base/scoped_ptr.h>

namespace streaming {
namespace f4v {

EsdsAtom::EsdsAtom()
  : VersionedAtom(kType),
    raw_data_(),
    mp4_es_desc_tag_id_(0),
    mp4_es_desc_tag_priority_(0),
    mp4_dec_config_desc_tag_object_type_id_(0),
    mp4_dec_config_desc_tag_stream_type_(0),
    mp4_dec_config_desc_tag_buffer_size_db_(0),
    mp4_dec_config_desc_tag_max_bit_rate_(0),
    mp4_dec_config_desc_tag_avg_bit_rate_(0),
    extra_data_() {
}
EsdsAtom::EsdsAtom(const EsdsAtom& other)
  : VersionedAtom(other),
    raw_data_(),
    mp4_es_desc_tag_id_(other.mp4_es_desc_tag_id_),
    mp4_es_desc_tag_priority_(other.mp4_es_desc_tag_priority_),
    mp4_dec_config_desc_tag_object_type_id_(other.mp4_dec_config_desc_tag_object_type_id_),
    mp4_dec_config_desc_tag_stream_type_(other.mp4_dec_config_desc_tag_stream_type_),
    mp4_dec_config_desc_tag_buffer_size_db_(other.mp4_dec_config_desc_tag_buffer_size_db_),
    mp4_dec_config_desc_tag_max_bit_rate_(other.mp4_dec_config_desc_tag_max_bit_rate_),
    mp4_dec_config_desc_tag_avg_bit_rate_(other.mp4_dec_config_desc_tag_avg_bit_rate_),
    extra_data_() {
  raw_data_.AppendStreamNonDestructive(&other.raw_data_);
  extra_data_.AppendStreamNonDestructive(&other.extra_data_);
}
EsdsAtom::~EsdsAtom() {
}

const io::MemoryStream& EsdsAtom::extra_data() const {
  return extra_data_;
}

void EsdsAtom::set_mp4_es_desc_tag_id(uint16 mp4_es_desc_tag_id) {
  mp4_es_desc_tag_id_ = mp4_es_desc_tag_id;
}
void EsdsAtom::set_mp4_es_desc_tag_priority(uint8 mp4_es_desc_tag_priority) {
  mp4_es_desc_tag_priority_ = mp4_es_desc_tag_priority;
}
void EsdsAtom::set_mp4_dec_config_desc_tag_object_type_id(uint8 mp4_dec_config_desc_tag_object_type_id) {
  mp4_dec_config_desc_tag_object_type_id_ = mp4_dec_config_desc_tag_object_type_id;
}
void EsdsAtom::set_mp4_dec_config_desc_tag_stream_type(uint8 mp4_dec_config_desc_tag_stream_type) {
  mp4_dec_config_desc_tag_stream_type_ = mp4_dec_config_desc_tag_stream_type;
}
void EsdsAtom::set_mp4_dec_config_desc_tag_buffer_size_db(uint32 mp4_dec_config_desc_tag_buffer_size_db) {
  mp4_dec_config_desc_tag_buffer_size_db_ = mp4_dec_config_desc_tag_buffer_size_db;
}
void EsdsAtom::set_mp4_dec_config_desc_tag_max_bit_rate(uint32 mp4_dec_config_desc_tag_max_bit_rate) {
  mp4_dec_config_desc_tag_max_bit_rate_ = mp4_dec_config_desc_tag_max_bit_rate;
}
void EsdsAtom::set_mp4_dec_config_desc_tag_avg_bit_rate(uint32 mp4_dec_config_desc_tag_avg_bit_rate) {
  mp4_dec_config_desc_tag_avg_bit_rate_ = mp4_dec_config_desc_tag_avg_bit_rate;
}
void EsdsAtom::set_extra_data(const io::MemoryStream& extra_data) {
  extra_data_.Clear();
  extra_data_.AppendStreamNonDestructive(&extra_data);
}
void EsdsAtom::set_extra_data(const void* extra_data, uint32 size) {
  extra_data_.Clear();
  extra_data_.Write(extra_data, size);
}

//static
TagDecodeStatus EsdsAtom::ReadTagTypeAndLength(io::MemoryStream& in,
                                               uint8& type,
                                               uint32& length) {
  type = io::NumStreamer::ReadByte(&in);
  length = 0;
  for ( int i = 0; i < 4; i++ ) {
    if ( in.Size() < 1 ) {
      return TAG_DECODE_NO_DATA;
    }
    uint8 b = io::NumStreamer::ReadByte(&in);
    length = (length << 7) | (b & 0x7f);
    if ( (b & 0x80) == 0 ) {
      break;
    }
  }
  return TAG_DECODE_SUCCESS;
}
//static
void EsdsAtom::WriteTagTypeAndLength(io::MemoryStream& out,
                                     uint8 type,
                                     uint32 length) {
  // The 'length' is written as 4 bytes with 7 bits each.
  // general format is: |1...b3...|1...b2...|1...b1...|0...b0...|
  //             where: length = b0 + b1 << 7 + b2 << 14 + b3 << 21;
  // e.g. for length = 0x9D8BA = 0000000 0100111 0110001 0111010
  //      We devide length in 7 bit parts.
  //      And the encoded length is: 10000000 10100111 10110001 00111010
  //                                 ^        ^        ^        ^
  //      If first bit is 1 => decoding continues
  //                      0 => this is last byte, decoding stops.
  CHECK_LT(length, 1 << 29);

  uint8 enc_len[4] = {0,};
  uint32 len = length;
  for ( int i = 3; i >= 0; i-- ) {
    uint8 b = len & 0x7f;
    len = (len >> 7) & 0x1fffff;
    enc_len[i] = b | 0x80; // first bit is generally 1
  }
  enc_len[3] &= 0x7f; // clear the first bit of the last byte
  io::NumStreamer::WriteByte(&out, type);
  out.Write(enc_len, 4);
}

void EsdsAtom::GetSubatoms(vector<const BaseAtom*>& subatoms) const {
}
BaseAtom* EsdsAtom::Clone() const {
  return new EsdsAtom(*this);
}
TagDecodeStatus EsdsAtom::DecodeVersionedBody(uint64 size,
                                              io::MemoryStream& in,
                                              Decoder& decoder) {
  // TEST ReadTagTypeAndLength WriteTagTypeAndLength
  /*
  {
    #define TEST_WRITE_READ(type, length, expected_ms_size) {\
      LOG_WARNING << "testing type: " << type << ", length: " << length;\
      io::MemoryStream ms;\
      WriteTagTypeAndLength(ms, type, length);\
      CHECK_EQ(ms.Size(), expected_ms_size);\
      uint8 out_type;\
      uint32 out_length;\
      TagDecodeStatus result = ReadTagTypeAndLength(ms, out_type, out_length);\
      CHECK(ms.IsEmpty()) << " Ms not empty";\
      CHECK_EQ(result, TAG_DECODE_SUCCESS) << "ReadTagTypeAndLength failed: " << result;\
      CHECK_EQ(out_type, type) << " Type mismatch";\
      CHECK_EQ(out_length, length) << " Length mismatch";\
    }
    TEST_WRITE_READ(0, 0, 5);
    TEST_WRITE_READ(1, 1, 5);
    TEST_WRITE_READ(3, 7, 5);
    TEST_WRITE_READ(13, 0x7f, 5);
    TEST_WRITE_READ(13, 0x80, 5);
    TEST_WRITE_READ(13, 0x81, 5);
    TEST_WRITE_READ(13, 0x3fff, 5);
    TEST_WRITE_READ(13, 0x4000, 5);
    TEST_WRITE_READ(13, 0x4001, 5);
    TEST_WRITE_READ(13, 0x1fffff, 5);
    TEST_WRITE_READ(13, 0x200000, 5);
    TEST_WRITE_READ(13, 0x200001, 5);
    TEST_WRITE_READ(13, 0xfffffff, 5);
  }
  */

  #define VERIFY_IN_SIZE(expected_size) {\
      if ( in.Size() < expected_size ) {\
      DATOMLOG << "Not enough data in stream: " << in.Size()\
               << " is less than expected: " << expected_size;\
      return TAG_DECODE_NO_DATA;\
    }\
  }

  VERIFY_IN_SIZE(size);
  DATOMLOG << "total body size: " << size;

  // put all the body in raw_data_. The rest of the decode is just to
  // find extra_data_ and for attributes printing.
  //
  raw_data_.Clear();
  raw_data_.AppendStreamNonDestructive(&in, size);

  const int32 begin_size = in.Size();
  uint8 tag_type = 0;
  uint32 tag_length = 0;
  TagDecodeStatus result = TAG_DECODE_SUCCESS;

  result = ReadTagTypeAndLength(in, tag_type, tag_length);
  if ( result != TAG_DECODE_SUCCESS ) {
    EATOMLOG << "ReadTagTypeAndLength failed";
    return result;
  }
  DATOMLOG << "sub tag, type: " << (uint32)tag_type
           << ", length: " << tag_length
           << ", remaining body size: " << (size - begin_size + in.Size());

  VERIFY_IN_SIZE(2);
  mp4_es_desc_tag_id_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  if ( tag_type == MP4ESDescrTag ) {
    VERIFY_IN_SIZE(1);
    mp4_es_desc_tag_priority_ = io::NumStreamer::ReadByte(&in);
  }

  result = ReadTagTypeAndLength(in, tag_type, tag_length);
  if ( result != TAG_DECODE_SUCCESS ) {
    EATOMLOG << "ReadTagTypeAndLength failed";
    return result;
  }
  DATOMLOG << "sub tag, type: " << (uint32)tag_type
           << ", length: " << tag_length
           << ", remaining body size: " << (size - begin_size + in.Size());

  if ( tag_type == MP4DecConfigDescrTag ) {
    VERIFY_IN_SIZE(13);

    mp4_dec_config_desc_tag_object_type_id_ = io::NumStreamer::ReadByte(&in);
    mp4_dec_config_desc_tag_stream_type_ = io::NumStreamer::ReadByte(&in);
    mp4_dec_config_desc_tag_buffer_size_db_ = io::NumStreamer::ReadUInt24(&in, common::BIGENDIAN);
    mp4_dec_config_desc_tag_max_bit_rate_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
    mp4_dec_config_desc_tag_avg_bit_rate_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);

    result = ReadTagTypeAndLength(in, tag_type, tag_length);
    if ( result != TAG_DECODE_SUCCESS ) {
      EATOMLOG << "ReadTagTypeAndLength failed";
      return result;
    }
    DATOMLOG << "sub tag, type: " << (uint32)tag_type
             << ", length: " << tag_length
             << ", remaining body size: " << (size - begin_size + in.Size());

    if (tag_type == MP4UnknownTag) {
      VERIFY_IN_SIZE(1);
      uint8 unknown = io::NumStreamer::ReadByte(&in);
      WATOMLOG << "MP4UnknownTag: " << (uint32)unknown;

      result = ReadTagTypeAndLength(in, tag_type, tag_length);
      if ( result != TAG_DECODE_SUCCESS ) {
        EATOMLOG << "ReadTagTypeAndLength failed";
        return result;
      }
    }

    if ( tag_type == MP4DecSpecificDescrTag ) {
      //iso14496-3
      //http://wiki.multimedia.cx/index.php?title=MPEG-4_Audio
      if ( in.Size() < tag_length ) {
        EATOMLOG << "Not enough data for MP4DecSpecificDescrTag extra data"
                    ", expected: " << tag_length << ", have: " << in.Size();
        return TAG_DECODE_NO_DATA;
      }
      extra_data_.AppendStream(&in, tag_length);

      #if false
      // decode extra_data. Not sure about correctness.
      //
      static const uint32 kSampleRates[] = {96000, 88200, 64000, 48000, 44100,
                                            32000, 24000, 22050, 16000, 12000,
                                            11025, 8000, 7350};

      uint8* tmp = new uint8[tag_length];
      scoped_ptr<uint8> auto_del_tmp(tmp);

      extra_data_.Peek(tmp, tag_length);

      io::BitArray ba;
      ba.Wrap(tmp, tag_length);

      uint8 object_type = ba.Read<uint8>(5);
      WATOMLOG << "object_type: " << (uint32)object_type;

      uint8 sample_rate = ba.Read<uint8>(4);
      WATOMLOG << "sample_rate: " << (uint32)sample_rate << " => "
               << (sample_rate < sizeof(kSampleRates) ?
                   kSampleRates[sample_rate] : 0);

      uint8 channels = ba.Read<uint8>(4);
      WATOMLOG << "channels: " << (uint32)channels;

      while (ba.Size() >= 21) {
        if (ba.Peek<uint16>(11) == 0x02b7) {
          ba.Skip(11);

          uint8 ext_object_type = ba.Read<uint8>(5);
          WATOMLOG << "ext_object_type: " << (uint32)ext_object_type;

          uint8 sbr = ba.Read<uint8>(1);
          WATOMLOG << "sbr: " << (uint32)sbr;

          uint8 ext_sample_rate = ba.Read<uint8>(4);
          WATOMLOG << "ext_sample_rate: " << (uint32)ext_sample_rate << " => "
                   << (ext_sample_rate < sizeof(kSampleRates) ?
                       kSampleRates[ext_sample_rate] : 0);

          WATOMLOG << "ext leftovers bits count: " << ba.Size();
          break;
        }
        ba.Skip(1);
      }
      WATOMLOG << "extra data leftovers bits count: " << ba.Size();
      #endif
    }
  }

  const int32 end_size = in.Size();
  CHECK_GE(begin_size, end_size);

  const int32 decoded_size = begin_size - end_size;
  CHECK_GE(size, decoded_size);

  const int32 decode_leftover_size = size - decoded_size;

  // skip what's left undecoded.
  if ( decode_leftover_size > 0 ) {
    WATOMLOG << "undecoded leftover bytes: "
             << in.DumpContentInline(decode_leftover_size);
    in.Skip(decode_leftover_size);
  }

  return TAG_DECODE_SUCCESS;
}
void EsdsAtom::EncodeVersionedBody(io::MemoryStream& out,
                                   Encoder& encoder) const {
  if ( !raw_data_.IsEmpty() ) {
    out.AppendStreamNonDestructive(&raw_data_);
    return;
  }
  // TODO(cosmin): the ESDS encoding is a mess, mostly hardcoded.
  //               Find some official documentation.
  WriteTagTypeAndLength(out, MP4ESDescrTag, 34);
  io::NumStreamer::WriteUInt16(&out, mp4_es_desc_tag_id_, common::BIGENDIAN);
  io::NumStreamer::WriteByte(&out, mp4_es_desc_tag_priority_);
  WriteTagTypeAndLength(out, MP4DecConfigDescrTag, 20);
  io::NumStreamer::WriteByte(&out, mp4_dec_config_desc_tag_object_type_id_);
  io::NumStreamer::WriteByte(&out, mp4_dec_config_desc_tag_stream_type_);
  io::NumStreamer::WriteUInt24(&out, mp4_dec_config_desc_tag_buffer_size_db_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(&out, mp4_dec_config_desc_tag_max_bit_rate_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(&out, mp4_dec_config_desc_tag_avg_bit_rate_, common::BIGENDIAN);
  WriteTagTypeAndLength(out, MP4DecSpecificDescrTag , 2);
  out.AppendStreamNonDestructive(&extra_data_);
  out.Write("\x06\x80\x80\x80\x01\x02", 6); // from 'backcountry.f4v'

}
uint64 EsdsAtom::MeasureVersionedBodySize() const {
  return raw_data_.Size();
}
string EsdsAtom::ToStringVersionedBody(uint32 indent) const {
  return strutil::StringPrintf("raw_data_(full atom body): %u bytes"
                               ", mp4_es_desc_tag_id_: %u"
                               ", mp4_es_desc_tag_priority_: %u"
                               ", mp4_dec_config_desc_tag_object_type_id_: %u"
                               ", mp4_dec_config_desc_tag_stream_type_: %u"
                               ", mp4_dec_config_desc_tag_buffer_size_db_: %u"
                               ", mp4_dec_config_desc_tag_max_bit_rate_: %u"
                               ", mp4_dec_config_desc_tag_avg_bit_rate_: %u"
                               ", extra_data_: %s",
                               raw_data_.Size(),
                               mp4_es_desc_tag_id_,
                               mp4_es_desc_tag_priority_,
                               mp4_dec_config_desc_tag_object_type_id_,
                               mp4_dec_config_desc_tag_stream_type_,
                               mp4_dec_config_desc_tag_buffer_size_db_,
                               mp4_dec_config_desc_tag_max_bit_rate_,
                               mp4_dec_config_desc_tag_avg_bit_rate_,
                               extra_data_.DumpContentInline().c_str());
}
}
}
