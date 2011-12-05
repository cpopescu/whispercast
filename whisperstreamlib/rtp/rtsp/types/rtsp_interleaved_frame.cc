#include <whisperstreamlib/rtp/rtsp/types/rtsp_interleaved_frame.h>

namespace streaming {
namespace rtsp {

InterleavedFrame::InterleavedFrame()
  : Message(kType),
    channel_(0),
    data_() {
}
InterleavedFrame::InterleavedFrame(const InterleavedFrame& other)
  : Message(kType),
    channel_(other.channel_),
    data_() {
  data_.AppendStreamNonDestructive(&other.data_);
}
InterleavedFrame::~InterleavedFrame() {
}

uint8 InterleavedFrame::channel() const {
  return channel_;
}
const io::MemoryStream& InterleavedFrame::data() const {
  return data_;
}
void InterleavedFrame::set_channel(uint8 channel) {
  channel_ = channel;
}
io::MemoryStream* InterleavedFrame::mutable_data() {
  return &data_;
}

string InterleavedFrame::ToString() const {
  return strutil::StringPrintf(
      "rtsp::InterleavedFrame{channel_: %d, data_: #%d bytes}",
      channel_, data_.Size());
}

} // namespace rtsp
} // namespace streaming
