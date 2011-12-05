
#include <whisperlib/common/io/ioutil.h>
#include <whisperstreamlib/rtp/rtp_packetization.h>
#include <whisperstreamlib/rtp/rtp_consts.h>
#include <whisperstreamlib/rtp/rtp_util.h>

namespace streaming {
namespace rtp {

const char* Packetizer::TypeName(Type t) {
  switch ( t ) {
    CONSIDER(H263);
    CONSIDER(H264);
    CONSIDER(MP4A);
    CONSIDER(MP3);
    CONSIDER(SPLIT);
  };
  LOG_FATAL << "Unknown packetizer type: " << t;
  return NULL;
}

Packetizer::Packetizer(Type t)
  : type_(t) {
}
Packetizer::~Packetizer() {
}
Packetizer::Type Packetizer::type() const {
  return type_;
}
const char* Packetizer::type_name() const {
  return TypeName(type());
}
string Packetizer::ToString() {
  return strutil::StringPrintf("Packetizer{type_: %s}", type_name());;
}

////////////////////////////////////////////////////////////////////////////

bool H263Packetizer::Packetize(const io::MemoryStream& cin,
                               vector<io::MemoryStream*>* out) {
  io::MemoryStream& in = const_cast<io::MemoryStream&>(cin);
  if ( in.Size() < 2 ) {
    RTP_LOG_ERROR << "h263 frame too small";
    return false;
  }
  // TODO(cosmin): find out what is this first byte!
  in.Skip(1);
  uint8 b0 = io::NumStreamer::ReadByte(&in);
  uint8 b1 = io::NumStreamer::ReadByte(&in);
  if ( b0 != 0 || b1 != 0 ) {
    RTP_LOG_ERROR << "h263 header mismatch: b0=" << (int)b0 << ", b1="
                  << (int)b1 << ", rest: " << in.DumpContentInline();
    return false;
  }

  in.MarkerSet();
  bool first = true;
  while ( !in.IsEmpty() ) {
    const uint32 size = min((uint32)in.Size(), kRtpMtu - 2);
    io::MemoryStream* p = new io::MemoryStream();
    // h263 header
    int p_bit = first ? 1 : 0;
    int v_bit = 0; // no pesky error resilience
    int plen = 0; // normally plen=0 for PSC packet
    int pebit = 0; // because plen=0
    uint16 h = ( p_bit << 10 ) |
               ( v_bit << 9  ) |
               ( plen  << 3  ) |
                 pebit;
    io::NumStreamer::WriteUInt16(p, h, common::BIGENDIAN);
    p->AppendStream(&in, size);
    out->push_back(p);
    first = false;
  }
  in.MarkerRestore();
  return true;
}

////////////////////////////////////////////////////////////////////////////

bool H264Packetizer::Packetize(const io::MemoryStream& cin,
                               vector<io::MemoryStream*>* out) {
  io::MemoryStream& in = const_cast<io::MemoryStream&>(cin);
  in.MarkerSet();
  if ( is_flv_format_ ) {
    // test for AVC_SEQUENCE_HEADER. If found, do not packetize, just ignore it.
    uint8 tmp[6] = {0,};
    in.Peek(tmp, sizeof(tmp));
    if ( tmp[0] == 0x17 && tmp[1] == 0 && tmp[2] == 0 && tmp[3] == 0 &&
         tmp[4] == 0    && tmp[5] == 1 ) {
      RTP_LOG_WARNING << "Skipping AVC_SEQUENCE_HEADER in packetizer";
      in.MarkerRestore();
      return true;
    }
    // regular video tag: 7 unknown bytes in each FLV tag
    in.Skip(7);
  }
  bool result = SplitNalu(in, is_flv_format_ ? 2 : 4, out);
  if ( result ) {
    CHECK(in.IsEmpty());
  }
  in.MarkerRestore();
  return result;
}
bool H264Packetizer::SplitNalu(io::MemoryStream& in,
    uint8 nalu_size_bytes,
    vector<io::MemoryStream*>* out) {
  // split NAL units
  while ( !in.IsEmpty() ) {
    if ( in.Size() < nalu_size_bytes ) {
      RTP_LOG_ERROR << "h264 frame too small";
      return false;
    }
    uint32 nalu_size = 0;
    if ( nalu_size_bytes == 4 ) {
      nalu_size = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
    } else if ( nalu_size_bytes == 2 ) {
      nalu_size = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
    } else {
      LOG_ERROR << "Unrecognized NALU size bytes: " << nalu_size_bytes;
      return false;
    }
    if ( nalu_size == 0 ) {
      // empty NALU
      continue;
    }
    if ( nalu_size > in.Size() ) {
      RTP_LOG_ERROR << "nalu_size too big: nalu_size: " << nalu_size
                    << " , stream size: " << in.Size();
      return false;
    }
    if ( nalu_size <= kRtpMtu ) {
      // Single NAL unit packet
      io::MemoryStream* p = new io::MemoryStream();
      p->AppendStream(&in, nalu_size);
      out->push_back(p);
      continue;
    }

    // FU-A Fragmentation Unit without interleaving
    uint8 nalu_hdr = io::NumStreamer::ReadByte(&in);
    //uint8 nalu_f = (nalu_hdr & 0x80) >> 7;
    //uint8 nalu_nri = (nalu_hdr & 0x60) >> 5;
    uint8 nalu_type = nalu_hdr & 0x1f;

    uint32 count = 0;
    uint32 consumed = 1; // the skipped byte
    while ( consumed < nalu_size ) {
      uint32 size = min(nalu_size - consumed, kRtpMtu - 2);
      bool first = (count == 0);
      bool last = (consumed + size == nalu_size);
      io::MemoryStream* p = new io::MemoryStream();
      // FU indicator
      io::NumStreamer::WriteByte(p, (nalu_hdr & 0x60) | 28);
      // FU header
      io::NumStreamer::WriteByte(p, (first ? 0x80 : 0x00 ) |
                                    (last ? 0x40 : 0x00 ) | nalu_type);
      p->AppendStream(&in, size);
      consumed += size;
      out->push_back(p);
      count++;
    }
  }
  return true;
}


////////////////////////////////////////////////////////////////////////////

bool Mp4aPacketizer::Packetize(const io::MemoryStream& cin,
                               vector<io::MemoryStream*>* out) {
  io::MemoryStream& in = const_cast<io::MemoryStream&>(cin);
  in.MarkerSet();
  if ( is_flv_format_ ) {
    // unknown 2 bytes in FLV container
    in.Skip(2);
  }
  while ( !in.IsEmpty() ) {
    const uint32 size = min((uint32)in.Size(), kRtpMtu - 4);
    io::MemoryStream* p = new io::MemoryStream();
    // AU headers length (bits)
    io::NumStreamer::WriteUInt16(p, 0x0010, common::BIGENDIAN);
    // for each AU length 13 bits + idx 3 bits
    io::NumStreamer::WriteUInt16(p, size << 3, common::BIGENDIAN);
    p->AppendStream(&in, size);
    out->push_back(p);
  }
  in.MarkerRestore();
  return true;
}

////////////////////////////////////////////////////////////////////////////

bool Mp3Packetizer::Packetize(const io::MemoryStream& cin,
                              vector<io::MemoryStream*>* out) {
  io::MemoryStream& in = const_cast<io::MemoryStream&>(cin);
  in.MarkerSet();
  if ( is_flv_format_ ) {
    // unknown byte in FLV container
    in.Skip(1);
  }
  while ( !in.IsEmpty() ) {
    const uint32 size = min((uint32)in.Size(), kRtpMtu - 4);
    io::MemoryStream* p = new io::MemoryStream();
    // MP3 header (4 bytes: 0000)
    p->Write("\x00\x00\x00\x00", 4);
    p->AppendStream(&in, size);
    out->push_back(p);
  }
  in.MarkerRestore();
  return true;
}

////////////////////////////////////////////////////////////////////////////

bool SplitPacketizer::Packetize(const io::MemoryStream& cin,
                                vector<io::MemoryStream*>* out) {
  io::MemoryStream& in = const_cast<io::MemoryStream&>(cin);
  in.MarkerSet();
  while ( !in.IsEmpty() ) {
    const uint32 size = min((uint32)in.Size(), kRtpMtu);
    io::MemoryStream* p = new io::MemoryStream();
    p->AppendStream(&in, size);
    out->push_back(p);
  }
  in.MarkerRestore();
  return true;
}

} // namespace rtp
} // namespace streaming

