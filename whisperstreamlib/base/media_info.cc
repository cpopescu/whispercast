#include <string.h>
#include <whisperlib/common/base/strutil.h>
#include <whisperlib/net/util/base64.h>
#include <whisperstreamlib/base/media_info.h>

namespace streaming {

const char* MediaInfo::Audio::FormatName(Format format) {
  switch ( format ) {
    CONSIDER(FORMAT_AAC);
    CONSIDER(FORMAT_MP3);
  }
  LOG_FATAL << "Unknown format: " << format;
  return "UNKNOWN";
}
MediaInfo::Audio::Audio()
  : format_((Format)-1),
    channels_(1),
    sample_rate_(0),
    mp4_language_(),
    aac_level_(0),
    aac_profile_(0),
    aac_flv_container_(false),
    mp3_flv_container_(false) {
  memset(aac_config_, 0, sizeof(aac_config_));
}
MediaInfo::Audio::~Audio() {
}
const char* MediaInfo::Audio::format_name() const {
  return FormatName(format_);
}
string MediaInfo::Audio::ToString() const {
  return strutil::StringPrintf("{format_: %s, channels_: %u, sample_rate_: %u"
      ", mp4_language_: %s, aac_level_: %u, aac_profile_: %u"
      ", aac_config_: %02x%02x, aac_flv_container_: %s"
      ", mp3_flv_container_: %s}",
      format_name(), channels_, sample_rate_, mp4_language_.c_str(),
      aac_level_, aac_profile_, aac_config_[0], aac_config_[1],
      strutil::BoolToString(aac_flv_container_).c_str(),
      strutil::BoolToString(mp3_flv_container_).c_str());
}

/////////////////////////////////////////////////////////////////////////////

const char* MediaInfo::Video::FormatName(Format format) {
  switch ( format ) {
    CONSIDER(FORMAT_H263);
    CONSIDER(FORMAT_H264);
    CONSIDER(FORMAT_ON_VP6);
  }
  LOG_FATAL << "Unknown format: " << format;
  return "UNKNOWN";
}

MediaInfo::Video::Video()
  : format_((Format)-1),
    width_(0),
    height_(0),
    clock_rate_(0),
    frame_rate_(0.0f),
    mp4_moov_position_(0),
    h264_pps_(),
    h264_sps_(),
    h264_profile_(0),
    h264_profile_compatibility_(0),
    h264_level_(0),
    h264_flv_container_(false) {
}

MediaInfo::Video::~Video() {
}
const char* MediaInfo::Video::format_name() const {
  return FormatName(format_);
}
string MediaInfo::Video::ToString() const {
  return strutil::StringPrintf("{format_: %s, width_: %u, height_: %u"
      ", clock_rate_: %u, frame_rate_: %.3f, mp4_moov_position_: %u"
      ", h264_pps_: %s, h264_sps_: %s, h264_profile_: %u"
      ", h264_profile_compatibility_: %u, h264_level_: %u"
      ", h264_flv_container_: %s}",
      format_name(), width_, height_, clock_rate_, frame_rate_,
      mp4_moov_position_, base64::EncodeVector(h264_pps_).c_str(),
      base64::EncodeVector(h264_sps_).c_str(),
      h264_profile_, h264_profile_compatibility_, h264_level_,
      strutil::BoolToString(h264_flv_container_).c_str());
}

/////////////////////////////////////////////////////////////////////////////

MediaInfo::MediaInfo()
  : audio_(), video_(), duration_ms_(0) {
}
MediaInfo::~MediaInfo() {
}

bool MediaInfo::has_audio() const {
  return audio_.format_ != -1;
}
bool MediaInfo::has_video() const {
  return video_.format_ != -1;
}

const MediaInfo::Audio& MediaInfo::audio() const {
  CHECK(has_audio());
  return audio_;
}
MediaInfo::Audio* MediaInfo::mutable_audio() {
  return &audio_;
}

const MediaInfo::Video& MediaInfo::video() const {
  CHECK(has_video());
  return video_;
}
MediaInfo::Video* MediaInfo::mutable_video() {
  return &video_;
}

uint32 MediaInfo::duration_ms() const {
  return duration_ms_;
}
void MediaInfo::set_duration_ms(uint32 duration_ms) {
  duration_ms_ = duration_ms;
}

string MediaInfo::ToString() const {
  return strutil::StringPrintf(
      "MediaInfo{audio_: %s, video_: %s, duration_ms_: %u}",
      (has_audio() ? audio_.ToString().c_str() : "NONE"),
      (has_video() ? video_.ToString().c_str() : "NONE"), duration_ms_);
}

}
