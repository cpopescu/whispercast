#include <whisperlib/common/base/date.h>
#include <whisperlib/common/base/scoped_ptr.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_encoder.h>

namespace streaming {
namespace rtsp {

void Encoder::Encode(const Message& msg, io::MemoryStream* out) {
  switch ( msg.type() ) {
    case Message::MESSAGE_REQUEST:
      EncodeRequest(static_cast<const Request&>(msg), out);
      return;
    case Message::MESSAGE_RESPONSE:
      EncodeResponse(static_cast<const Response&>(msg), out);
      return;
    case Message::MESSAGE_INTERLEAVED_FRAME:
      EncodeInterleavedFrame(static_cast<const InterleavedFrame&>(msg), out);
      return;
  }
  LOG_FATAL << "Illegal Message type: " << msg.type();
}
void Encoder::EncodeRequest(const Request& request, io::MemoryStream* out) {
  CHECK(request.CheckContent()) << " " << request.ToString();
  EncodeHeader(request.header(), out);
  EncodeBody(request.data(), out);
}
void Encoder::EncodeResponse(const Response& response, io::MemoryStream* out) {
  CHECK(response.CheckContent()) << " " << response.ToString();
  EncodeHeader(response.header(), out);
  EncodeBody(response.data(), out);
}
void Encoder::EncodeInterleavedFrame(const InterleavedFrame& frame,
    io::MemoryStream* out) {
  EncodeInterleavedFrame(frame.channel(), frame.data(), out);
}
void Encoder::EncodeInterleavedFrame(uint8 channel,
    const io::MemoryStream& data, io::MemoryStream* out) {
  // Interleaved packet:
  //  ${1 byte channel}{2 byte length}{"length" bytes data, w/RTP header}
  if ( data.Size() >= kMaxUInt16 ) {
    RTSP_LOG_ERROR << "InteleavedFrame too large: " << data.Size()
                   << ", cannot encode";
    return;
  }
  io::NumStreamer::WriteByte(out, '$');
  io::NumStreamer::WriteByte(out, channel);
  io::NumStreamer::WriteUInt16(out, data.Size(), common::BIGENDIAN);
  out->AppendStreamNonDestructive(&data);
}

string Encoder::StrEncodeRequestLine(REQUEST_METHOD method, const URL* url,
    uint8 ver_hi, uint8 ver_lo) {
  const uint32 size = 32 + (url == NULL ? 0 : url->spec().size());
  char* buf = new char[size];
  scoped_ptr<char> auto_del_buf(buf);
  int write = snprintf(buf, size, "%s %s RTSP/%hhu.%hhu\r\n",
      RequestMethodEnc(method).c_str(),
      url == NULL ? "*" : url->spec().c_str(),
      ver_hi, ver_lo);
  CHECK_LT(write, size);
  return string(buf);
}
string Encoder::StrEncodeResponseLine(STATUS_CODE status_code,
    uint8 ver_hi, uint8 ver_lo) {
  char buf[128] = {0,};
  int write = snprintf(buf, sizeof(buf), "RTSP/%hhu.%hhu %d %s\r\n",
      ver_hi, ver_lo, status_code, StatusCodeDescription(status_code));
  CHECK_LT(write, sizeof(buf));
  return string(buf);
}

/////////////////////////////////////////////////////////////////////////////
void Encoder::EncodeHeader(const RequestHeader& header, io::MemoryStream* out) {
  out->Write(StrEncodeRequestLine(header.method(), header.url(),
      header.version_hi(), header.version_lo()));
  EncodeBaseHeader(header, out);
}
void Encoder::EncodeHeader(const ResponseHeader& header, io::MemoryStream* out){
  out->Write(StrEncodeResponseLine(header.status(),
      header.version_hi(), header.version_lo()));
  EncodeBaseHeader(header, out);
}
void Encoder::EncodeBaseHeader(const BaseHeader& header, io::MemoryStream* out){
  for ( BaseHeader::FieldMap::const_iterator it = header.fields().begin();
        it != header.fields().end(); ++it ) {
    const HeaderField* field = it->second;
    out->Write(field->name());
    out->Write(": ");
    out->Write(field->StrEncode());
    out->Write("\r\n");
  }
  out->Write("\r\n");
}
void Encoder::EncodeBody(const io::MemoryStream& body, io::MemoryStream* out) {
  if ( body.IsEmpty() ) {
    return;
  }
  out->AppendStreamNonDestructive(&body);
}

} // namespace rtsp
} // namespace streaming
