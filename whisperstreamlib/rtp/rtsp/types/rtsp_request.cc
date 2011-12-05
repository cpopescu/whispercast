
#include <whisperstreamlib/rtp/rtsp/rtsp_common.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_connection.h>
#include <whisperstreamlib/rtp/rtsp/types/rtsp_request.h>

namespace streaming {
namespace rtsp {

Request::Request(REQUEST_METHOD method)
  : HeadedMessage<RequestHeader>(kType),
    connection_(NULL){
  header_.set_method(method);
}
Request::Request(const Request& other)
   : HeadedMessage<RequestHeader>(other),
     connection_(other.connection_){
}
Request::~Request() {
}

Connection* Request::connection() const {
  return connection_;
}
void Request::set_connection(Connection* connection) const {
  connection_ = connection;
}

string Request::ToString() const {
  return strutil::StringPrintf(
      "rtsp::Request{header_: %s, data_: #%d bytes, connection_: %s}",
      header_.ToString().c_str(), data_.Size(),
      connection_ == NULL ? "NULL" : connection_->ToString().c_str());
}

} // namespace rtsp
} // namespace streaming
