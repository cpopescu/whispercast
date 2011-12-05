
#include <whisperlib/common/base/scoped_ptr.h>
#include <whisperlib/common/io/file/file.h>
#include <whisperlib/common/io/file/file_input_stream.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperstreamlib/rtp/rtsp/types/rtsp_header_field.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_encoder.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_decoder.h>

DEFINE_string(sample_messages,
              "",
              "File containing RTSP messages");

#define LOG_TEST LOG(-1)

uint32 RandomUInt(uint32 max = kMaxUInt32) {
  return ::rand() % max;
}
string RandomString() {
  uint32 len = 1 + RandomUInt(32);
  char tmp[34] = {0,};
  static const char abc[] =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-+=_";
  static const uint32 abc_len = ::strlen(abc);
  for ( uint32 i = 0; i < len; i++ ) {
    tmp[i] = abc[RandomUInt(abc_len)];
  }
  return tmp;
}

char* MsContent(const io::MemoryStream& ms) {
  char* sz_ms = new char[ms.Size()+1];
  ms.Peek(sz_ms, ms.Size());
  sz_ms[ms.Size()] = 0;
  return sz_ms;
}

void TestMessage(io::MemoryStream& ms, bool is_request) {
  CHECK(!ms.IsEmpty());
  char* sz_ms = MsContent(ms);
  scoped_ptr<char> auto_del_sz_ms(sz_ms);

  streaming::rtsp::Decoder dec;
  streaming::rtsp::Message* msg = NULL;
  streaming::rtsp::DecodeStatus result = dec.Decode(ms, &msg);
  CHECK(result == streaming::rtsp::DECODE_SUCCESS)
    << " Failed to decode rtsp message from: [" << sz_ms << "]";
  CHECK_NOT_NULL(msg);
  CHECK_EQ(msg->type(), is_request ? streaming::rtsp::Message::MESSAGE_REQUEST
                                   : streaming::rtsp::Message::MESSAGE_RESPONSE);
  CHECK(ms.IsEmpty()) << " Extra bytes: " << ms.Size();
  io::MemoryStream out;
  streaming::rtsp::Encoder::Encode(*msg, &out);
  char* sz_out = MsContent(out);
  scoped_ptr<char> auto_del_sz_out(sz_out);
  streaming::rtsp::Message* msg2 = NULL;
  result = dec.Decode(out, &msg2);
  CHECK(result == streaming::rtsp::DECODE_SUCCESS)
      << " Failed to decode rtsp message from: [" << sz_out << "]"
      << " Obtained from msg: " << msg->ToString()
      << " Obtained from ms: [" << sz_ms << "]";
  CHECK(*msg == *msg2)
       << " Msg1: " << msg->ToString()
       << " obtained from: [" << sz_ms << "]"
       << " differs from Msg2: " << msg2->ToString()
       << " obtained from: [" << sz_out << "]";
}

template<typename T>
void TestHeaderField(const T& field) {
  string s = field.StrEncode();
  T field2(field);
  CHECK(field2.Decode(s)) << " Failed to decode: [" << s << "]";
  CHECK(field2 == field) << " field2: " << field2.ToString()
                         << " decoded from: [" << s << "]"
                         << " differs from field: " << field.ToString();
  LOG_INFO << "Pass: " << field.ToString();
}

int main(int argc, char** argv) {
  common::Init(argc, argv);

  ////////////////////////////////////////////////////////////////
  // test header fields
  TestHeaderField(streaming::rtsp::ContentLengthHeaderField(RandomUInt()));
  TestHeaderField(streaming::rtsp::BandwidthHeaderField(RandomUInt()));
  TestHeaderField(streaming::rtsp::BlocksizeHeaderField(RandomUInt()));
  TestHeaderField(streaming::rtsp::CSeqHeaderField(RandomUInt()));
  TestHeaderField(streaming::rtsp::SessionHeaderField(RandomString(), RandomUInt()));

  TestHeaderField(streaming::rtsp::DateHeaderField(timer::Date(2010,7,28,22,13,1,0,true)));
  TestHeaderField(streaming::rtsp::ExpiresHeaderField(timer::Date(2010,7,28,22,13,1,0,true)));
  TestHeaderField(streaming::rtsp::IfModifiedSinceHeaderField(timer::Date(2010,7,28,22,13,1,0,true)));
  TestHeaderField(streaming::rtsp::LastModifiedHeaderField(timer::Date(2010,7,28,22,13,1,0,true)));

  TestHeaderField(streaming::rtsp::AcceptHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::AcceptEncodingHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::AcceptLanguageHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::AllowHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::AuthorizationHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::CacheControlHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::ConferenceHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::ConnectionHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::ContentBaseHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::ContentEncodingHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::ContentLanguageHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::ContentLocationHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::ContentTypeHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::FromHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::HostHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::IfMatchHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::LocationHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::ProxyAuthenticateHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::ProxyRequireHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::PublicHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::RangeHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::RefererHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::RetryAfterHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::RequireHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::RtpInfoHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::ScaleHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::SpeedHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::ServerHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::TimestampHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::UnsupportedHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::UserAgentHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::VaryHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::ViaHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::WwwAuthenticateHeaderField(RandomString()));
  TestHeaderField(streaming::rtsp::CustomHeaderField("my_custom_field", RandomString()));
  streaming::rtsp::TransportHeaderField thf;
  thf.set_transport("RTP");
  thf.set_profile("AVP");
  thf.set_transmission_type(streaming::rtsp::TransportHeaderField::TT_MULTICAST);
  thf.set_mode("PLAY");
  thf.set_append(true);
  // the rest of the parameters are optional
  TestHeaderField(thf);
  thf.set_lower_transport("UDP");
  TestHeaderField(thf);
  thf.set_destination("127.0.0.2");
  TestHeaderField(thf);
  thf.set_source("1.0.0.127");
  TestHeaderField(thf);
  thf.set_layers(3);
  TestHeaderField(thf);
  thf.set_interleaved(make_pair(1,2));
  TestHeaderField(thf);
  thf.set_ttl(12);
  TestHeaderField(thf);
  thf.set_port(make_pair(1234, 1235));
  TestHeaderField(thf);
  thf.set_client_port(make_pair(1236, 1237));
  TestHeaderField(thf);
  thf.set_server_port(make_pair(1238, 1239));
  TestHeaderField(thf);
  thf.set_ssrc(RandomString());
  TestHeaderField(thf);

  LOG_INFO << "Pass ALL HeaderField tests";
  ////////////////////////////////////////////////////////////////////////////

  if ( FLAGS_sample_messages == "" ) {
    LOG_ERROR << "No sample_messages FLAG";
    common::Exit(1);
  }

  io::File* f = new io::File();
  if ( !f->Open(FLAGS_sample_messages.c_str(), io::File::GENERIC_READ,
      io::File::OPEN_EXISTING) ) {
    LOG_ERROR << "Failed to open file: [" << FLAGS_sample_messages << "]"
                 ", err: " << GetLastSystemErrorDescription();
    common::Exit(1);
  }

  io::FileInputStream in(f);

  io::MemoryStream ms;
  bool is_request = false;
  uint32 nline = 0;
  uint32 msg_start_line = 0;
  while ( !in.IsEos() ) {
    nline++;
    string line;
    in.ReadLine(&line);

    if ( strutil::StrStartsWith(line, "////") ) {
      if ( !ms.IsEmpty() ) {
        LOG_TEST << "Testing " << (is_request ? "Request" : "Response")
                 << " at line: " << msg_start_line;
        TestMessage(ms, is_request);
      }
      ms.Clear();
      is_request = strutil::StrStartsWith(line, "//// Request");
      msg_start_line = nline;
      continue;
    }
    ms.Write(line);
    ms.Write("\r\n");
  }

  LOG_INFO << "Pass";
  common::Exit(0);
}
