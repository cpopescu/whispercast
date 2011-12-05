#include <whisperstreamlib/rtp/rtsp/rtsp_common.h>
#include <whisperstreamlib/rtp/rtsp/types/rtsp_header_field.h>

#define ILOG_DEBUG   RTSP_LOG_DEBUG << type_name() << ": "
#define ILOG_INFO    RTSP_LOG_INFO << type_name() << ": "
#define ILOG_WARNING RTSP_LOG_WARNING << type_name() << ": "
#define ILOG_ERROR   RTSP_LOG_ERROR << type_name() << ": "
#define ILOG_FATAL   RTSP_LOG_FATAL << type_name() << ": "

namespace streaming {
namespace rtsp {

const char* HeaderField::TypeName(Type type) {
  switch ( type ) {
    CONSIDER(HF_ACCEPT);
    CONSIDER(HF_ACCEPT_ENCODING);
    CONSIDER(HF_ACCEPT_LANGUAGE);
    CONSIDER(HF_ALLOW);
    CONSIDER(HF_AUTHORIZATION);
    CONSIDER(HF_BANDWIDTH);
    CONSIDER(HF_BLOCKSIZE);
    CONSIDER(HF_CACHE_CONTROL);
    CONSIDER(HF_CONFERENCE);
    CONSIDER(HF_CONNECTION);
    CONSIDER(HF_CONTENT_BASE);
    CONSIDER(HF_CONTENT_ENCODING);
    CONSIDER(HF_CONTENT_LANGUAGE);
    CONSIDER(HF_CONTENT_LENGTH);
    CONSIDER(HF_CONTENT_LOCATION);
    CONSIDER(HF_CONTENT_TYPE);
    CONSIDER(HF_CSEQ);
    CONSIDER(HF_DATE);
    CONSIDER(HF_EXPIRES);
    CONSIDER(HF_FROM);
    CONSIDER(HF_HOST);
    CONSIDER(HF_IF_MATCH);
    CONSIDER(HF_IF_MODIFIED_SINCE);
    CONSIDER(HF_LAST_MODIFIED);
    CONSIDER(HF_LOCATION);
    CONSIDER(HF_PROXY_AUTHENTICATE);
    CONSIDER(HF_PROXY_REQUIRE);
    CONSIDER(HF_PUBLIC);
    CONSIDER(HF_RANGE);
    CONSIDER(HF_REFERER);
    CONSIDER(HF_RETRY_AFTER);
    CONSIDER(HF_REQUIRE);
    CONSIDER(HF_RTP_INFO);
    CONSIDER(HF_SCALE);
    CONSIDER(HF_SPEED);
    CONSIDER(HF_SERVER);
    CONSIDER(HF_SESSION);
    CONSIDER(HF_TIMESTAMP);
    CONSIDER(HF_TRANSPORT);
    CONSIDER(HF_UNSUPPORTED);
    CONSIDER(HF_USER_AGENT);
    CONSIDER(HF_VARY);
    CONSIDER(HF_VIA);
    CONSIDER(HF_WWW_AUTHENTICATE);
    CONSIDER(HF_CUSTOM);
  }
  RTSP_LOG_FATAL << "Illegal HeaderField Type: " << type;
  return "Unknown";
}
const string& HeaderField::TypeEnc(Type hf) {
  switch ( hf ) {
    case HF_ACCEPT: return kHeaderFieldAccept;
    case HF_ACCEPT_ENCODING: return kHeaderFieldAcceptEncoding;
    case HF_ACCEPT_LANGUAGE: return kHeaderFieldAcceptLanguage;
    case HF_ALLOW: return kHeaderFieldAllow;
    case HF_AUTHORIZATION: return kHeaderFieldAuthorization;
    case HF_BANDWIDTH: return kHeaderFieldBandwidth;
    case HF_BLOCKSIZE: return kHeaderFieldBlocksize;
    case HF_CACHE_CONTROL: return kHeaderFieldCacheControl;
    case HF_CONFERENCE: return kHeaderFieldConference;
    case HF_CONNECTION: return kHeaderFieldConnection;
    case HF_CONTENT_BASE: return kHeaderFieldContentBase;
    case HF_CONTENT_ENCODING: return kHeaderFieldContentEncoding;
    case HF_CONTENT_LANGUAGE: return kHeaderFieldContentLanguage;
    case HF_CONTENT_LENGTH: return kHeaderFieldContentLength;
    case HF_CONTENT_LOCATION: return kHeaderFieldContentLocation;
    case HF_CONTENT_TYPE: return kHeaderFieldContentType;
    case HF_CSEQ: return kHeaderFieldCSeq;
    case HF_DATE: return kHeaderFieldDate;
    case HF_EXPIRES: return kHeaderFieldExpires;
    case HF_FROM: return kHeaderFieldFrom;
    case HF_HOST: return kHeaderFieldHost;
    case HF_IF_MATCH: return kHeaderFieldIfMatch;
    case HF_IF_MODIFIED_SINCE: return kHeaderFieldIfModifiedSince;
    case HF_LAST_MODIFIED: return kHeaderFieldLastModified;
    case HF_LOCATION: return kHeaderFieldLocation;
    case HF_PROXY_AUTHENTICATE: return kHeaderFieldProxyAuthenticate;
    case HF_PROXY_REQUIRE: return kHeaderFieldProxyRequire;
    case HF_PUBLIC: return kHeaderFieldPublic;
    case HF_RANGE: return kHeaderFieldRange;
    case HF_REFERER: return kHeaderFieldReferer;
    case HF_RETRY_AFTER: return kHeaderFieldRetryAfter;
    case HF_REQUIRE: return kHeaderFieldRequire;
    case HF_RTP_INFO: return kHeaderFieldRtpInfo;
    case HF_SCALE: return kHeaderFieldScale;
    case HF_SPEED: return kHeaderFieldSpeed;
    case HF_SERVER: return kHeaderFieldServer;
    case HF_SESSION: return kHeaderFieldSession;
    case HF_TIMESTAMP: return kHeaderFieldTimestamp;
    case HF_TRANSPORT: return kHeaderFieldTransport;
    case HF_UNSUPPORTED: return kHeaderFieldUnsupported;
    case HF_USER_AGENT: return kHeaderFieldUserAgent;
    case HF_VARY: return kHeaderFieldVary;
    case HF_VIA: return kHeaderFieldVia;
    case HF_WWW_AUTHENTICATE: return kHeaderFieldWwwAuthenticate;
    case HF_CUSTOM: break; // fall through
  }
  RTSP_LOG_FATAL << "Illegal HeaderField Type: " << hf;
  static const string kUnknown("Unknown");
  return kUnknown;
}
HeaderField::Type HeaderField::TypeDec(const string& hf) {
  if ( hf == kHeaderFieldAccept ) { return HF_ACCEPT; };
  if ( hf == kHeaderFieldAcceptEncoding ) { return HF_ACCEPT_ENCODING; };
  if ( hf == kHeaderFieldAcceptLanguage ) { return HF_ACCEPT_LANGUAGE; };
  if ( hf == kHeaderFieldAllow ) { return HF_ALLOW; };
  if ( hf == kHeaderFieldAuthorization ) { return HF_AUTHORIZATION; };
  if ( hf == kHeaderFieldBandwidth ) { return HF_BANDWIDTH; };
  if ( hf == kHeaderFieldBlocksize ) { return HF_BLOCKSIZE; };
  if ( hf == kHeaderFieldCacheControl ) { return HF_CACHE_CONTROL; };
  if ( hf == kHeaderFieldConference ) { return HF_CONFERENCE; };
  if ( hf == kHeaderFieldConnection ) { return HF_CONNECTION; };
  if ( hf == kHeaderFieldContentBase ) { return HF_CONTENT_BASE; };
  if ( hf == kHeaderFieldContentEncoding ) { return HF_CONTENT_ENCODING; };
  if ( hf == kHeaderFieldContentLanguage ) { return HF_CONTENT_LANGUAGE; };
  if ( hf == kHeaderFieldContentLength ) { return HF_CONTENT_LENGTH; };
  if ( hf == kHeaderFieldContentLocation ) { return HF_CONTENT_LOCATION; };
  if ( hf == kHeaderFieldContentType ) { return HF_CONTENT_TYPE; };
  if ( hf == kHeaderFieldCSeq ) { return HF_CSEQ; };
  if ( hf == kHeaderFieldDate ) { return HF_DATE; };
  if ( hf == kHeaderFieldExpires ) { return HF_EXPIRES; };
  if ( hf == kHeaderFieldFrom ) { return HF_FROM; };
  if ( hf == kHeaderFieldHost ) { return HF_HOST; };
  if ( hf == kHeaderFieldIfMatch ) { return HF_IF_MATCH; };
  if ( hf == kHeaderFieldIfModifiedSince ) { return HF_IF_MODIFIED_SINCE; };
  if ( hf == kHeaderFieldLastModified ) { return HF_LAST_MODIFIED; };
  if ( hf == kHeaderFieldLocation ) { return HF_LOCATION; };
  if ( hf == kHeaderFieldProxyAuthenticate ) { return HF_PROXY_AUTHENTICATE; };
  if ( hf == kHeaderFieldProxyRequire ) { return HF_PROXY_REQUIRE; };
  if ( hf == kHeaderFieldPublic ) { return HF_PUBLIC; };
  if ( hf == kHeaderFieldRange ) { return HF_RANGE; };
  if ( hf == kHeaderFieldReferer ) { return HF_REFERER; };
  if ( hf == kHeaderFieldRetryAfter ) { return HF_RETRY_AFTER; };
  if ( hf == kHeaderFieldRequire ) { return HF_REQUIRE; };
  if ( hf == kHeaderFieldRtpInfo ) { return HF_RTP_INFO; };
  if ( hf == kHeaderFieldScale ) { return HF_SCALE; };
  if ( hf == kHeaderFieldSpeed ) { return HF_SPEED; };
  if ( hf == kHeaderFieldServer ) { return HF_SERVER; };
  if ( hf == kHeaderFieldSession ) { return HF_SESSION; };
  if ( hf == kHeaderFieldTimestamp ) { return HF_TIMESTAMP; };
  if ( hf == kHeaderFieldTransport ) { return HF_TRANSPORT; };
  if ( hf == kHeaderFieldUnsupported ) { return HF_UNSUPPORTED; };
  if ( hf == kHeaderFieldUserAgent ) { return HF_USER_AGENT; };
  if ( hf == kHeaderFieldVary ) { return HF_VARY; };
  if ( hf == kHeaderFieldVia ) { return HF_VIA; };
  if ( hf == kHeaderFieldWwwAuthenticate ) { return HF_WWW_AUTHENTICATE; };
  return HF_CUSTOM;
}

int HeaderField::ParseInt(const char* s, uint32 len) {
  int a = 0;
  for ( int i = 0; i < len && s[i] != 0; i++ ) {
    a = a * 10 + s[i] - '0';
  }
  return a;
}

string HeaderField::StrEncode() const {
  string s;
  Encode(&s);
  return s;
}
string HeaderField::ToString() const {
  return strutil::StringPrintf("%s{%s}", type_name(), ToStringBase().c_str());
}

/////////////////////////////////////////////////////////////////////////////

TransportHeaderField::TransportHeaderField()
  : HeaderField(kType, TypeEnc(kType)),
    transport_(),
    profile_(),
    lower_transport_(),
    transmission_type_(TT_MULTICAST),
    destination_(),
    source_(),
    layers_(),
    mode_(),
    append_(false),
    interleaved_(),
    ttl_(),
    port_(),
    client_port_(),
    server_port_(),
    ssrc_() {
}
TransportHeaderField::~TransportHeaderField() {
}

const string& TransportHeaderField::transport() const {
  return transport_;
}
const string& TransportHeaderField::profile() const {
  return profile_;
}
const OptionalAttribute<string>& TransportHeaderField::lower_transport() const {
  return lower_transport_;
}
TransportHeaderField::TransmissionType TransportHeaderField::transmission_type() const {
  return transmission_type_;
}
const OptionalAttribute<string>& TransportHeaderField::destination() const {
  return destination_;
}
const OptionalAttribute<string>& TransportHeaderField::source() const {
  return source_;
}
const OptionalAttribute<int>& TransportHeaderField::layers() const {
  return layers_;
}
const OptionalAttribute<string>& TransportHeaderField::mode() const {
  return mode_;
}
bool TransportHeaderField::append() const {
  return append_;
}
const OptionalAttribute<pair<int, int> >& TransportHeaderField::interleaved() const {
  return interleaved_;
}
const OptionalAttribute<int>& TransportHeaderField::ttl() const {
  return ttl_;
}
const OptionalAttribute<pair<int, int> >& TransportHeaderField::port() const {
  return port_;
}
const OptionalAttribute<pair<int, int> >& TransportHeaderField::client_port() const {
  return client_port_;
}
const OptionalAttribute<pair<int, int> >& TransportHeaderField::server_port() const {
  return server_port_;
}
const OptionalAttribute<string>& TransportHeaderField::ssrc() const {
  return ssrc_;
}

void TransportHeaderField::set_transport(const string& transport) {
  transport_ = transport;
}
void TransportHeaderField::set_profile(const string& profile) {
  profile_ = profile;
}
void TransportHeaderField::set_lower_transport(const string& lower_transport) {
  lower_transport_ = lower_transport;
}
void TransportHeaderField::set_transmission_type(TransmissionType transmission_type) {
  transmission_type_ = transmission_type;
}
void TransportHeaderField::set_destination(const string& destination) {
  destination_ = destination;
}
void TransportHeaderField::set_source(const string& source) {
  source_ = source;
}
void TransportHeaderField::set_layers(const int layers) {
  layers_ = layers;
}
void TransportHeaderField::set_mode(const string& mode) {
  mode_ = mode;
}
void TransportHeaderField::set_append(bool append) {
  append_ = append;
}
void TransportHeaderField::set_interleaved(pair<int, int> interleaved) {
  interleaved_ = interleaved;
}
void TransportHeaderField::set_ttl(int ttl) {
  ttl_ = ttl;
}
void TransportHeaderField::set_port(pair<int, int> port) {
  port_ = port;
}
void TransportHeaderField::set_client_port(pair<int, int> client_port) {
  client_port_ = client_port;
}
void TransportHeaderField::set_server_port(pair<int, int> server_port) {
  server_port_ = server_port;
}
void TransportHeaderField::set_ssrc(const string& ssrc) {
  ssrc_ = ssrc;
}

bool TransportHeaderField::operator==(const HeaderField& other) const {
  if ( !HeaderField::operator==(other) ) {
    return false;
  }
  const TransportHeaderField& other_transport =
      static_cast<const TransportHeaderField&>(other);
  return transport_ == other_transport.transport_ &&
         profile_ == other_transport.profile_ &&
         lower_transport_ == other_transport.lower_transport_ &&
         transmission_type_ == other_transport.transmission_type_ &&
         destination_ == other_transport.destination_ &&
         source_ == other_transport.source_ &&
         layers_ == other_transport.layers_ &&
         mode_ == other_transport.mode_ &&
         append_ == other_transport.append_ &&
         interleaved_ == other_transport.interleaved_ &&
         ttl_ == other_transport.ttl_ &&
         port_ == other_transport.port_ &&
         client_port_ == other_transport.client_port_ &&
         server_port_ == other_transport.server_port_ &&
         ssrc_ == other_transport.ssrc_;
}

pair<int,int> TransportHeaderField::ParseRange(const string& s) {
  int index = s.find('-');
  if ( index == -1 ) {
    return make_pair(0,0);
  }
  return make_pair(ParseInt(s.c_str(), index),
                   ParseInt(s.c_str() + index + 1));
}
string TransportHeaderField::Unquote(const string& s) {
  if ( s == "" ||
       s[0] != '\"' ) {
    return s;
  }
  if ( s[s.size()-1] != '\"' ) {
    return s.substr(1);
  }
  return s.substr(1, s.size() - 2);
}
string TransportHeaderField::StrRange(const pair<int, int>& r) {
  char s[64] = {0, };
  snprintf(s, sizeof(s), "%d-%d", r.first, r.second);
  return s;
}
bool TransportHeaderField::Decode(const string& in) {
  vector<string> trans;
  strutil::SplitString(in, ";", &trans);
  if ( trans.size() == 0 ) {
    ILOG_ERROR << "Failed to parse transport header: [" << in << "]";
    return false;
  }
  vector<string> tpl;
  strutil::SplitString(trans[0], "/", &tpl);
  if ( tpl.size() != 2 && tpl.size() != 3 ) {
    ILOG_ERROR << "Illegal transport spec: [" << trans[0] << "]"
               << "] in transport header: [" << in << "]";
    return false;
  }
  transport_ = tpl[0];
  profile_ = tpl[1];
  if ( tpl.size() == 3 ) {
    lower_transport_.set(tpl[2]);
  }
  for ( uint32 i = 1; i < trans.size(); i++ ) {
    const string& param = trans[i];
    if ( param == "" ) {
      continue; // it happens on '..a;;b..' or on '..a;'
    }
    const pair<string, string> p = strutil::SplitFirst(param.c_str(), '=');
    if ( p.second == "" ) {
      if ( p.first == "unicast" ) {
        transmission_type_ = TT_UNICAST;
        continue;
      }
      if ( p.first == "multicast" ) {
        transmission_type_ = TT_MULTICAST;
        continue;
      }
      if ( p.first == "append" ) {
        append_ = true;
        continue;
      }
      ILOG_ERROR << "Unknown option: [" << param << "].. ignoring";
      continue;
    }
    if ( p.first == "destination" ) {
      destination_.set(p.second);
      continue;
    }
    if ( p.first == "source" ) {
      source_.set(p.second);
      continue;
    }
    if ( p.first == "layers" ) {
      layers_.set(ParseInt(p.second.c_str()));
      continue;
    }
    if ( p.first == "mode" ) {
      mode_.set(Unquote(p.second));
      continue;
    }
    if ( p.first == "interleaved" ) {
      interleaved_.set(ParseRange(p.second));
      continue;
    }
    if ( p.first == "ttl" ) {
      ttl_.set(ParseInt(p.second.c_str()));
      continue;
    }
    if ( p.first == "port" ) {
      port_.set(ParseRange(p.second));
      continue;
    }
    if ( p.first == "client_port" ) {
      client_port_.set(ParseRange(p.second));
      continue;
    }
    if ( p.first == "server_port" ) {
      server_port_.set(ParseRange(p.second));
      continue;
    }
    if ( p.first == "ssrc" ) {
      ssrc_.set(p.second);
      continue;
    }
    ILOG_ERROR << "Unknown option: [" << param << "].. ignoring";
    continue;
  }
  return true;
}
void TransportHeaderField::Encode(string* out) const {
  ostringstream oss;
  oss << transport_ << "/" << profile_;
  if ( lower_transport_.is_set() ) {
    oss << "/" << lower_transport_.get();
  }
  switch ( transmission_type_ ) {
    case TT_MULTICAST:
      oss << ";multicast";
      break;
    case TT_UNICAST:
      oss << ";unicast";
      break;
  }
  if ( destination_.is_set() ) {
    oss << ";destination=" << destination_.get();
  }
  if ( source_.is_set() ) {
    oss << ";source=" << source_.get();
  }
  if ( layers_.is_set() ) {
    oss << ";layers=" << layers_.get();
  }
  if ( mode_.is_set() ) {
    oss << ";mode=\"" << mode_.get() << "\"";
  }
  if ( append_ ) {
    oss << ";append";
  }
  if ( interleaved_.is_set() ) {
    oss << ";interleaved=" << StrRange(interleaved_.get());
  }
  if ( ttl_.is_set() ) {
    oss << ";ttl=" << ttl_.get();
  }
  if ( port_.is_set() ) {
    oss << ";port=" << StrRange(port_.get());
  }
  if ( client_port_.is_set() ) {
    oss << ";client_port=" << StrRange(client_port_.get());
  }
  if ( server_port_.is_set() ) {
    oss << ";server_port=" << StrRange(server_port_.get());
  }
  if ( ssrc_.is_set() ) {
    oss << ";ssrc=" << ssrc_.get();
  }
  *out = oss.str();
}

string TransportHeaderField::ToStringBase() const {
  return StrEncode();
}

////////////////////////////////////////////////////////////////////////

SessionHeaderField::SessionHeaderField()
  : HeaderField(kType, TypeEnc(kType)),
    id_(),
    timeout_(0) {
}
SessionHeaderField::SessionHeaderField(const string& id, uint32 timeout)
  : HeaderField(kType, TypeEnc(kType)),
    id_(id),
    timeout_(timeout) {
}
SessionHeaderField::~SessionHeaderField() {
}

const string& SessionHeaderField::id() const {
  return id_;
}
uint32 SessionHeaderField::timeout() const {
  return timeout_;
}
void SessionHeaderField::set_id(const string& id) {
  id_ = id;
}
void SessionHeaderField::set_timeout(uint32 timeout) {
  timeout_ = timeout;
}

bool SessionHeaderField::operator==(const HeaderField& other) const {
  if ( !HeaderField::operator==(other) ) {
    return false;
  }
  const SessionHeaderField& other_session =
      static_cast<const SessionHeaderField&>(other);
  return id_ == other_session.id_ &&
         timeout_ == other_session.timeout_;
}

bool SessionHeaderField::Decode(const string& in) {
  vector<string> v;
  strutil::SplitString(in, ";", &v);

  bool id_found = false;
  bool timeout_found = false;
  for ( uint32 i = 0; i < v.size(); i++ ) {
    const string& s = v[i];
    if ( s == "" ) {
      // useless: ";;"
      continue;
    }
    if ( strutil::StrStartsWith(s, "timeout=") ) {
      // timeout
      if ( timeout_found ) {
        RTSP_LOG_ERROR << "Multiple timeout in session id: [" << in << "]";
        return false;
      }
      timeout_ = ::atol(s.c_str() + 8);
      timeout_found = true;
      continue;
    }
    // id
    if ( id_found ) {
      RTSP_LOG_ERROR << "Illegal session id: [" << in << "]";
      return false;
    }
    id_ = s;
    id_found = true;
  }
  if ( !id_found ) {
    RTSP_LOG_ERROR << "No session id found in: [" << in << "]";
    return false;
  }
  return true;
}
void SessionHeaderField::Encode(string* out) const {
  *out = id_;
  if ( timeout_ != 0 ) {
    *out += strutil::StringPrintf(";timeout=%d", timeout_);
  }
}
string SessionHeaderField::ToStringBase() const {
  return StrEncode();
}

} // namespace rtsp
} // namespace streaming

ostream& operator<<(ostream& os, const streaming::rtsp::HeaderField& field) {
  return os << field.ToString();
}
