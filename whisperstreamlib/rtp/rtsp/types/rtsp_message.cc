
#include <whisperstreamlib/rtp/rtsp/types/rtsp_request.h>
#include <whisperstreamlib/rtp/rtsp/types/rtsp_response.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_common.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_connection.h>

namespace streaming {
namespace rtsp {

const char* Message::TypeName(Type type) {
  switch(type) {
    CONSIDER(MESSAGE_REQUEST);
    CONSIDER(MESSAGE_RESPONSE);
    CONSIDER(MESSAGE_INTERLEAVED_FRAME);
  };
  return "==UNKNOWN==";  // keep gcc happy
}
Message::Message(Type type)
  : type_(type) {
}
Message::Message(const Message& other)
  : type_(other.type_) {
}
Message::~Message() {
}
const Message::Type Message::type() const {
  return type_;
}
const char* Message::type_name() const {
  return TypeName(type());
}

bool Message::operator==(const Message& other) const {
  return type() == other.type();
}

} // namespace rtsp
} // namespace streaming
