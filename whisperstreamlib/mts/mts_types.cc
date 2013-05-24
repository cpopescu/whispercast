
#include <whisperstreamlib/mts/mts_types.h>

#define CHECK_MARKER(bit_array, type, bit_size, expected_value) {\
    type marker = bit_array.Read<type>(bit_size);\
    if ( marker != expected_value ) {\
      LOG_ERROR << "Invalid marker: " << (int)marker\
                << ", expected: " << (int)expected_value;\
    }\
}
#define CHECK_MARKER_BIT(bit_array) CHECK_MARKER(bit_array, uint8, 1, 0x01);
#define CHECK_ZERO_BIT(bit_array) CHECK_MARKER(bit_array, uint8, 1, 0x00);

namespace streaming {
namespace mts {

const char* TSScramblingName(TSScrambling scrambling) {
  switch ( scrambling ) {
    CONSIDER(NOT_SCRAMBLED);
    CONSIDER(SCRAMBLED_RESERVED);
    CONSIDER(SCRAMBLED_EVEN_KEY);
    CONSIDER(SCRAMBLED_ODD_KEY);
  }
  LOG_FATAL << "Illegal TSScrambling: " << (int)scrambling;
  return "Unknown";
}

DecodeStatus TSPacket::Decode(io::MemoryStream* in) {
  if ( in->Size() < kTSPacketSize ) {
    return DECODE_NO_DATA;
  }
  if ( io::NumStreamer::PeekByte(in) != kTSSyncByte ) {
    LOG_ERROR << "Cannot decode TSPacket, sync byte: "
              << strutil::ToHex(kTSSyncByte)
              << " not found in stream: " << in->DumpContentInline(16);
    return DECODE_ERROR;
  }
  in->Skip(1); // skip the sync byte

  io::BitArray h;
  h.PutMS(*in, 3);
  transport_error_indicator_ = h.Read<bool>(1);
  payload_unit_start_indicator_ = h.Read<bool>(1);
  transport_priority_ = h.Read<bool>(1);
  pid_ = h.Read<uint16>(13);
  scrambling_ = (TSScrambling)h.Read<uint8>(2);
  bool has_adaptation = h.Read<bool>(1);
  bool has_payload = h.Read<bool>(1);
  continuity_counter_ = h.Read<uint8>(4);
  if ( has_adaptation ) {
    adaptation_field_ = new AdaptationField();
    DecodeStatus status = adaptation_field_->Decode(in);
    if ( status != DECODE_SUCCESS ) {
      LOG_ERROR << "Cannot decode TSPacket, failed to decode AdaptationField"
                   ", result: " << DecodeStatusName(status);
      return status;
    }
  }
  if ( has_payload ) {
    payload_ = new io::MemoryStream();
    // the header, before adaptation field, has 4 bytes.
    payload_->AppendStream(in, kTSPacketSize - 4 -
        (has_adaptation ? adaptation_field_->Size() : 0));
  }
  return DECODE_SUCCESS;
}
string TSPacket::ToString() const {
  ostringstream oss;
  oss << "TSPacket{"
      "transport_error_indicator_: " << strutil::BoolToString(transport_error_indicator_)
  << ", payload_unit_start_indicator_: " << strutil::BoolToString(payload_unit_start_indicator_)
  << ", transport_priority_: " << strutil::BoolToString(transport_priority_)
  << ", pid_: " << pid_
  << ", scrambling_: " << TSScramblingName(scrambling_)
  << ", continuity_counter_: " << (uint32)continuity_counter_;
  if ( adaptation_field_ == NULL ) {
    oss << ", adaptation_field_: NULL";
  } else {
    oss << ", adaptation_field: " << adaptation_field_->ToString();
  }
  if ( payload_ == NULL ) {
    oss << ", payload_: NULL";
  } else {
    oss << ", payload_: " << payload_->Size() << " bytes";
  }
  oss << "}";
  return oss.str();
}

////////////////////////////////////////////////////////////////////////

DecodeStatus ProgramAssociationTable::Decode(io::MemoryStream* in) {
  if ( in->Size() < 8 ) {
    return DECODE_NO_DATA;
  }
  io::BitArray data;
  data.PutMS(*in, 8);

  table_id_ = data.Read<uint8>(8);
  CHECK_MARKER_BIT(data); // 1 bit: section_syntax_indicator, must be 1
  CHECK_ZERO_BIT(data);
  data.Skip(2); // 2 reserved bits
  uint16 section_length = data.Read<uint16>(12);
  ts_id_ = data.Read<uint16>(16);
  data.Skip(2); // 2 reserved bits
  version_number_ = data.Read<uint8>(5);
  currently_applicable_ = data.Read<bool>(1);
  section_number_ = data.Read<uint8>(8);
  last_section_number_ = data.Read<uint8>(8);

  // section_length is the length of the remaining section data
  // (starting with ts_id_ and including the last CRC).
  // So we've already decoded 5 bytes from this length.

  if ( section_length < 9 ) {
    LOG_ERROR << "Cannot decode ProgramAssociationTable, invalid section_length"
                 ": " << section_length << " is too small";
    return DECODE_ERROR;
  }
  if ( in->Size() < section_length - 5 ) {
    return DECODE_NO_DATA;
  }
  if ( (section_length - 5) % 4 ) {
    LOG_ERROR << "Cannot decode ProgramAssociationTable, invalid section_length"
                 ": " << section_length << ", after the header there are: "
              << section_length - 5 << " bytes, which is not a multiple of 4";
    return DECODE_ERROR;
  }
  for ( uint32 i = 5; i < section_length - 4; i += 4 ) {
    uint16 program_number = io::NumStreamer::ReadUInt16(in, common::BIGENDIAN);
    io::BitArray a;
    a.PutMS(*in, 16);
    a.Skip(3);
    uint16 pid = a.Read<uint16>(13);
    if ( program_number == 0 ) {
      network_pid_[program_number] = pid;
    } else {
      program_map_pid_[program_number] = pid;
    }
  }
  crc_ = io::NumStreamer::ReadUInt32(in, common::BIGENDIAN);
  return DECODE_SUCCESS;
}

string ProgramAssociationTable::ToString() const {
  ostringstream oss;
  oss << "ProgramAssociationTable{table_id_: " << (uint32)table_id_
      << ", ts_id_: " << ts_id_
      << ", version_number_: " << version_number_
      << ", currently_applicable_: " << strutil::BoolToString(currently_applicable_)
      << ", section_number_: " << section_number_
      << ", last_section_number_: " << last_section_number_
      << ", network_pid_: " << strutil::ToString(network_pid_)
      << ", program_map_pid_: " << strutil::ToString(program_map_pid_)
      << ", crc_: " << crc_
      << "}";
  return oss.str();
}

////////////////////////////////////////////////////////////////////////

DecodeStatus PESPacket::Header::Decode(io::MemoryStream* in) {
  if ( in->Size() < 2 ) {
    return DECODE_NO_DATA;
  }
  io::BitArray h;
  h.PutMS(*in, 1);
  uint8 mark = h.Read<uint8>(2);
  scrambling_ = (TSScrambling)h.Read<uint8>(2);
  is_priority_ = h.Read<bool>(1);
  is_data_aligned_ = h.Read<bool>(1);
  is_copyright_ = h.Read<bool>(1);
  is_original_ = h.Read<bool>(1);
  bool has_PTS = h.Read<bool>(1);
  bool has_DTS = h.Read<bool>(1);
  bool has_ESCR = h.Read<bool>(1);
  bool has_ES_rate = h.Read<bool>(1);
  bool has_DSM_trick_mode = h.Read<bool>(1);
  bool has_additional_copy_info = h.Read<bool>(1);
  bool has_CRC = h.Read<bool>(1);
  bool has_extension = h.Read<bool>(1);
  uint8 size = io::NumStreamer::ReadByte(in);
  uint32 start_size = in->Size();

  if ( mark != 0b01 ) {
    LOG_ERROR << "Invalid marker before PES: " << strutil::ToBinary(mark);
    return DECODE_ERROR;
  }

  if ( has_PTS && !has_DTS ) {
    io::BitArray data;
    data.PutMS(*in, 5);
    pts_ = 0; // Note: 33 bit encoded!
    CHECK_MARKER(data, uint8, 4, 0b0010);
    pts_ = pts_ << 3 | data.Read<uint8>(3);
    CHECK_MARKER_BIT(data);
    pts_ = pts_ << 15 | data.Read<uint16>(15);
    CHECK_MARKER_BIT(data);
    pts_ = pts_ << 15 | data.Read<uint16>(15);
    CHECK_MARKER_BIT(data);
  } else if ( has_PTS && has_DTS ) {
    io::BitArray data;
    data.PutMS(*in, 10);
    pts_ = 0; // Note: 33 bit encoded!
    dts_ = 0; // Note: 33 bit encoded!
    CHECK_MARKER(data, uint8, 4, 0b0011);
    pts_ = pts_ << 3 | data.Read<uint8>(3);
    CHECK_MARKER_BIT(data);
    pts_ = pts_ << 15 | data.Read<uint16>(15);
    CHECK_MARKER_BIT(data);
    pts_ = pts_ << 15 | data.Read<uint16>(15);
    CHECK_MARKER_BIT(data);
    CHECK_MARKER(data, uint8, 4, 0b0001);
    dts_ = dts_ << 3 | data.Read<uint8>(3);
    CHECK_MARKER_BIT(data);
    dts_ = dts_ << 15 | data.Read<uint16>(15);
    CHECK_MARKER_BIT(data);
    dts_ = dts_ << 15 | data.Read<uint16>(15);
    CHECK_MARKER_BIT(data);
  } else if ( !has_PTS && has_DTS ) {
    LOG_ERROR << "Found Decoding Timestamp without Presentation Timestamp";
    return DECODE_ERROR;
  }
  if ( has_ESCR ) {
    io::BitArray data;
    data.PutMS(*in, 6);
    escr_base_ = 0;
    data.Skip(2); // reserved
    escr_base_ = escr_base_ << 3 | data.Read<uint8>(3);
    CHECK_MARKER_BIT(data);
    escr_base_ = escr_base_ << 15 | data.Read<uint16>(15);
    CHECK_MARKER_BIT(data);
    escr_base_ = escr_base_ << 15 | data.Read<uint16>(15);
    CHECK_MARKER_BIT(data);
    escr_extension_ = data.Read<uint16>(9);
    CHECK_MARKER_BIT(data);
  }
  if ( has_ES_rate ) {
    io::BitArray data;
    data.PutMS(*in, 3);
    CHECK_MARKER_BIT(data);
    es_rate_ = data.Read<uint32>(22);
    CHECK_MARKER_BIT(data);
  }
  if ( has_DSM_trick_mode ) {
    // trick mode is encoded on 8 bits.
    // TODO(cosmin): decode DSM trick mode
    trick_mode_ = io::NumStreamer::ReadByte(in);
  }
  if ( has_additional_copy_info ) {
    // additional copy info is encoded on 8 bits. Ignore it for now.
    // TODO(cosmin): decode additional copy info
    additional_copy_info_ = io::NumStreamer::ReadByte(in);
  }
  if ( has_CRC ) {
    previous_pes_packet_crc_ = io::NumStreamer::ReadUInt16(in, common::BIGENDIAN);
  }
  if ( has_extension ) {
    // TODO(cosmin): decode extension
  }
  // skip the rest of the header (extension + stuffing bytes)
  uint32 decoded_size = start_size - in->Size();
  in->Skip(size - decoded_size);
  return DECODE_SUCCESS;
}
string PESPacket::Header::ToString() const {
  ostringstream oss;
  oss << "{scrambling_: " << TSScramblingName(scrambling_)
      << ", is_priority_: " << strutil::BoolToString(is_priority_)
      << ", is_data_aligned_: " << strutil::BoolToString(is_data_aligned_)
      << ", is_copyright_: " << strutil::BoolToString(is_copyright_)
      << ", is_original_: " << strutil::BoolToString(is_original_)
      << ", pts_: " << pts_
      << ", dts_: " << dts_
      << ", escr_base_: " << escr_base_
      << ", escr_extension_: " << escr_extension_
      << ", es_rate_: " << es_rate_
      << ", trick_mode_: " << strutil::ToHex(trick_mode_)
      << ", add_info: " << strutil::ToHex(additional_copy_info_)
      << ", prev_crc: " << previous_pes_packet_crc_
      << "}";
  return oss.str();
}

DecodeStatus PESPacket::Decode(io::MemoryStream* in) {
  if ( in->Size() < 6 ) {
    return DECODE_NO_DATA;
  }
  in->MarkerSet();
  uint32 mark = io::NumStreamer::ReadUInt24(in, common::BIGENDIAN);
  if ( mark != kPESMarker ) {
    in->MarkerRestore();
    LOG_ERROR << "PES Marker not found."
                 " Expected: " << strutil::ToHex(kPESMarker, 3)
              << ", found: " << strutil::ToHex(mark, 3)
              << ", in stream: " << in->DumpContentInline(16);
    return DECODE_ERROR;
  }
  uint8 stream_id = io::NumStreamer::ReadByte(in);
  uint16 size = io::NumStreamer::ReadUInt16(in, common::BIGENDIAN);
  if ( in->Size() < size ) {
    in->MarkerRestore();
    return DECODE_NO_DATA;
  }
  if ( stream_id == kPESStreamIdPadding ) {
    in->Skip(size);
    in->MarkerClear();
    return DECODE_SUCCESS;
  }
  if ( stream_id == kPESStreamIdProgramMap ||
       stream_id == kPESStreamIdProgramDirectory ||
       stream_id == kPESStreamIdECM ||
       stream_id == kPESStreamIdEMM ||
       stream_id == kPESStreamIdPrivate2 ) {
    // decode body (no header)
    data_.AppendStream(in, size);
    in->MarkerClear();
    return DECODE_SUCCESS;
  }
  if ( (stream_id >= kPESStreamIdAudioStart && stream_id <= kPESStreamIdAudioEnd) ||
       (stream_id >= kPESStreamIdVideoStart && stream_id <= kPESStreamIdVideoEnd) ) {
    // decode header
    const uint32 start_size = in->Size();
    DecodeStatus status = header_.Decode(in);
    if ( status != DECODE_SUCCESS ) {
      in->MarkerRestore();
      return status;
    }
    const uint32 header_encoding_size = start_size - in->Size();
    CHECK_GE(size, header_encoding_size);
    // decode body
    data_.AppendStream(in, size - header_encoding_size);
    in->MarkerClear();
    return DECODE_SUCCESS;
  }
  LOG_ERROR << "Unknown stream ID: " << strutil::StringPrintf("%02x", stream_id);
  in->Skip(size);
  in->MarkerClear();
  return DECODE_SUCCESS;
}

string PESPacket::ToString() const {
  ostringstream oss;
  oss << "PESPacket{stream_id_: " << PESStreamIdName(stream_id_)
      << ", header_: " << header_.ToString()
      << ", data_: #" << data_.Size() << " bytes}";
  return oss.str();
}

}
}
