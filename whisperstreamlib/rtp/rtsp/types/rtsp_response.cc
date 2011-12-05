
#include <whisperstreamlib/rtp/rtsp/rtsp_common.h>
#include <whisperstreamlib/rtp/rtsp/types/rtsp_response.h>

namespace streaming {
namespace rtsp {

Response::Response(STATUS_CODE status)
  : HeadedMessage<ResponseHeader>(kType) {
  header_.set_status(status);
}
Response::Response(const Response& other)
  : HeadedMessage<ResponseHeader>(other) {
}
Response::~Response() {
}

} // namespace rtsp
} // namespace streaming
