#include <whisperstreamlib/rtp/rtp_header.h>

namespace streaming {
namespace rtp {

Header::Header()
  : flags_(0),
    payload_type_(0),
    sequence_number_(0),
    timestamp_(0),
    ssrc_(0) {
}
Header::~Header() {
}
void Header::set_flag_version(uint8 two_bit_version) {
  flags_ = ((two_bit_version & 0x03) << 6) | (flags_ & 0x3f);
}
void Header::set_flag_padding(uint8 one_bit_padding) {
  flags_ = ((one_bit_padding & 0x01) << 5) | (flags_ & 0xdf);
}
void Header::set_flag_extension(uint8 one_bit_extension) {
  flags_ = ((one_bit_extension & 0x01) << 4) | (flags_ & 0xef);
}
void Header::set_flag_cssid(uint8 four_bit_cssid) {
  flags_ = ((four_bit_cssid & 0x0f) << 0) | (flags_ & 0xf0);
}
void Header::set_payload_type_marker(uint8 one_bit_marker) {
  payload_type_ = ((one_bit_marker & 0x01) << 7) | (flags_ & 0x7f);
}
void Header::set_payload_type (uint8 seven_bit_payload_type) {
  payload_type_ = ((seven_bit_payload_type & 0x7f) << 0) |
                  (payload_type_ & 0x80);
}
void Header::set_sequence_number(uint16 sequence_number) {
  sequence_number_ = sequence_number;
}
void Header::set_timestamp(uint32 timestamp) {
  timestamp_ = timestamp;
}
void Header::set_ssrc(uint32 ssrc) {
  ssrc_ = ssrc;
}
void Header::Encode(io::MemoryStream* out) {
  io::NumStreamer::WriteByte(out, flags_);
  io::NumStreamer::WriteByte(out, payload_type_);
  io::NumStreamer::WriteUInt16(out, sequence_number_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(out, timestamp_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(out, ssrc_, common::BIGENDIAN);
}

} // namespace rtp
} // namespace streaming
