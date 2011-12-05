
#include <whisperstreamlib/rtp/rtsp/rtsp_common.h>
#include <whisperstreamlib/rtp/rtsp/types/rtsp_header.h>

namespace streaming {
namespace rtsp {

BaseHeader::BaseHeader(BaseHeader::Type type)
  : type_(type),
    version_hi_(kRtspVersionHi),
    version_lo_(kRtspVersionLo),
    fields_() {
}
BaseHeader::BaseHeader(const BaseHeader& other)
  : type_(other.type_),
    version_hi_(other.version_hi_),
    version_lo_(other.version_lo_),
    fields_(other.fields_) {
}
BaseHeader::~BaseHeader() {
  for ( FieldMap::iterator it = fields_.begin(); it != fields_.end(); ++it ) {
    HeaderField* f = it->second;
    delete f;
  }
  fields_.clear();
}
BaseHeader::Type BaseHeader::type() const {
  return type_;
}
uint8 BaseHeader::version_hi() const {
  return version_hi_;
}
uint8 BaseHeader::version_lo() const {
  return version_lo_;
}

void BaseHeader::add_field(HeaderField* field) {
  const HeaderField* old = get_field(field->name());
  CHECK_NULL(old) << " Field already exists: " << old->ToString();
  fields_[field->name()] = field;
}
const HeaderField* BaseHeader::get_field(const string& field_name) const {
  FieldMap::const_iterator it = fields_.find(field_name);
  return it == fields_.end() ? NULL : it->second;
}
HeaderField* BaseHeader::get_field(const string& field_name) {
  FieldMap::iterator it = fields_.find(field_name);
  return it == fields_.end() ? NULL : it->second;
}
const BaseHeader::FieldMap& BaseHeader::fields() const {
  return fields_;
}
bool BaseHeader::operator==(const BaseHeader& other) const {
  if ( type_ != other.type_ ||
       version_hi_ != other.version_hi_ ||
       version_lo_ != other.version_lo_ ||
       fields_.size() != other.fields_.size() ) {
    return false;
  }
  for ( FieldMap::const_iterator it = fields_.begin();
        it != fields_.end(); ++it ) {
    const HeaderField* hf = it->second;
    const HeaderField* other_hf = other.get_field(hf->name());
    if ( other_hf == NULL || !((*hf) == (*other_hf)) ) {
      return false;
    }
  }
  return true;
}

string BaseHeader::ToStringBase() const {
  vector<const HeaderField*> fv;
  for ( FieldMap::const_iterator it = fields_.begin(); it != fields_.end(); ++it ) {
    fv.push_back(it->second);
  }
  return strutil::StringPrintf("rtsp_version_: %hhu.%hhu, fields_: %s",
      version_hi(), version_lo(), strutil::ToStringP(fv).c_str());
}

//////////////////////////////////////////////////////////////////////////
RequestHeader::RequestHeader()
  : BaseHeader(kType),
    method_(METHOD_DESCRIBE),
    url_(NULL) {
}
RequestHeader::RequestHeader(const RequestHeader& other)
   : BaseHeader(other),
     method_(other.method_),
     url_(other.url_ == NULL ? NULL : new URL(*other.url_)) {
}
RequestHeader::~RequestHeader() {
  delete url_;
  url_ = NULL;
}
REQUEST_METHOD RequestHeader::method() const {
  return method_;
}
const URL* RequestHeader::url() const {
  return url_;
}
string RequestHeader::media() const {
  // +1 to skip the first slash
  return url_ == NULL ? "*" : url_->path().substr(1);
}
void RequestHeader::set_method(REQUEST_METHOD method) {
  method_ = method;
}
void RequestHeader::set_url(URL* url) {
  delete url_;
  url_ = url;
}
bool RequestHeader::operator==(const BaseHeader& other) const {
  if ( !BaseHeader::operator==(other) ) {
    return false;
  }
  const RequestHeader& other_request = static_cast<const RequestHeader&>(other);
  return method_ == other_request.method_ &&
         ((url_ == NULL && other_request.url_ == NULL) ||
          (url_->spec() == other_request.url_->spec()));
}
string RequestHeader::GetQueryParam(const string& name) const {
  if ( url_ == NULL ) {
    return "";
  }
  // TODO(cosmin): optimize with a local cache for url_query_params.
  //               However: this function is usually called once per request.
  vector<pair<string, string> > url_query_params;
  url_->GetQueryParameters(&url_query_params, true);
  for ( uint32 i = 0; i < url_query_params.size(); i++ ) {
    if ( url_query_params[i].first == name ) {
      return url_query_params[i].second;
    }
  }
  return "";
}
string RequestHeader::ToString() const {
  return strutil::StringPrintf("{method_: %s, url_: %s, %s}",
      RequestMethodName(method_),
      (url_ == NULL ? "*" : url_->spec().c_str()),
      ToStringBase().c_str());
}
//////////////////////////////////////////////////////////////////////////
ResponseHeader::ResponseHeader()
  : BaseHeader(kType),
    status_(STATUS_SEE_OTHER) {
}
ResponseHeader::ResponseHeader(const ResponseHeader& other)
  : BaseHeader(other),
    status_(other.status_) {
}
ResponseHeader::~ResponseHeader() {
}
STATUS_CODE ResponseHeader::status() const {
  return status_;
}
const char* ResponseHeader::status_name() const {
  return StatusCodeName(status_);
}
void ResponseHeader::set_status(STATUS_CODE status) {
  status_ = status;
}
bool ResponseHeader::operator==(const BaseHeader& other) const {
  if ( !BaseHeader::operator==(other) ) {
    return false;
  }
  const ResponseHeader& other_response =
      static_cast<const ResponseHeader&>(other);
  return status_ == other_response.status_;
}
string ResponseHeader::ToString() const {
  return strutil::StringPrintf("{status_: %s, %s}",
      StatusCodeName(status_), ToStringBase().c_str());
}

} // namespace rtsp
} // namespace streaming
