#include <string.h>
#include <whisperlib/common/base/strutil.h>
#include <whisperlib/net/util/base64.h>
#include <whisperstreamlib/base/media_info.h>
#include <whisperstreamlib/f4v/atoms/movie/moov_atom.h>
#include <whisperstreamlib/rtmp/objects/rtmp_objects.h>

namespace streaming {

const char* MediaInfo::Audio::FormatName(Format format) {
  switch ( format ) {
    CONSIDER(FORMAT_AAC);
    CONSIDER(FORMAT_MP3);
  }
  LOG_FATAL << "Unknown format: " << format;
  return "UNKNOWN";
}
string MediaInfo::Frame::ToString() const {
  std::ostringstream oss;
  oss << "MediaInfo::Frame{is_audio_: " << strutil::BoolToString(is_audio_)
      << ", size_: " << size_
      << ", decoding_ts_: " << decoding_ts_
      << ", composition_offset_ms_: " << composition_offset_ms_
      << ", is_keyframe_: " << strutil::BoolToString(is_keyframe_)
      << "}";
  return oss.str();
}
MediaInfo::Audio::Audio()
  : format_((Format)-1),
    channels_(1),
    sample_rate_(0),
    sample_size_(0),
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
string MediaInfo::Audio::ToString(const char* glue) const {
  ostringstream oss;
  oss << "{format_: " << format_name() << glue
      << "channels_: " << (int)channels_ << glue
      << "sample_rate_: " << sample_rate_ << glue
      << "sample_size_: " << sample_size_ << glue
      << "mp4_language_: " << mp4_language_ << glue
      << "aac_level_: " << aac_level_ << glue
      << "aac_profile_: " << (int)aac_profile_ << glue
      << "aac_config_: " << strutil::StringPrintf("{0x%02x, 0x%02x}",
            aac_config_[0], aac_config_[1]) << glue
      << "aac_flv_container_: " << strutil::BoolToString(aac_flv_container_) << glue
      << "mp3_flv_container_: " << strutil::BoolToString(mp3_flv_container_) << "}";
  return oss.str();
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
    bps_(0),
    timescale_(0),
    mp4_moov_position_(0),
    h264_configuration_version_(0),
    h264_profile_(0),
    h264_profile_compatibility_(0),
    h264_level_(0),
    h264_nalu_length_size_(0),
    h264_sps_(),
    h264_pps_(),
    h264_flv_container_(false),
    h264_nalu_start_code_(false),
    h264_avcc_() {
}

MediaInfo::Video::~Video() {
}
const char* MediaInfo::Video::format_name() const {
  return FormatName(format_);
}
namespace {
string Base64VectorToString(const vector<string>& v) {
  ostringstream oss;
  oss << "Vector<base64 string>#" << v.size() << "{";
  for ( uint32 i = 0; i < v.size(); i++ ) {
    if ( i > 0 ) { oss << ", "; }
    oss << base64::EncodeString(v[i]);
  }
  oss << "}";
  return oss.str();
}
}
string MediaInfo::Video::ToString(const char* glue) const {
  ostringstream oss;
  oss << "{format_: " << format_name() << glue
      << "width_: " << width_ << glue
      << "height_: " << height_ << glue
      << "clock_rate_: " << clock_rate_ << glue
      << "frame_rate_: " << frame_rate_ << glue
      << "timescale_: " << timescale_ << glue
      << "mp4_moov_position_: " << mp4_moov_position_ << glue
      << "h264_configuration_version_: " << (int)h264_configuration_version_ << glue
      << "h264_profile_: " << (int)h264_profile_ << glue
      << "h264_profile_compatibility_: " << (int)h264_profile_compatibility_ << glue
      << "h264_level_: " << (int)h264_level_ << glue
      << "h264_pps_: " << Base64VectorToString(h264_pps_) << glue
      << "h264_sps_: " << Base64VectorToString(h264_sps_) << glue
      << "h264_flv_container_: " << strutil::BoolToString(h264_flv_container_) << glue
      << "h264_nalu_start_code_: " << strutil::BoolToString(h264_nalu_start_code_) << glue
      << "h264_avcc_: " << strutil::PrintableDataBufferInline(
            h264_avcc_.c_str(), h264_avcc_.size()) << "}";
  return oss.str();
}

/////////////////////////////////////////////////////////////////////////////

MediaInfo::MediaInfo()
  : audio_(), video_(), duration_ms_(0), file_size_(0), seekable_(true),
    pausable_(true), frames_(),
    flv_extra_metadata_(),
    mp4_moov_(NULL) {
}
MediaInfo::MediaInfo(const MediaInfo& other)
  : audio_(other.audio_),
    video_(other.video_),
    duration_ms_(other.duration_ms_),
    file_size_(other.file_size_),
    seekable_(other.seekable_),
    pausable_(other.pausable_),
    frames_(other.frames_),
    flv_extra_metadata_(other.flv_extra_metadata_),
    mp4_moov_(other.mp4_moov_ == NULL ? NULL :
              new f4v::MoovAtom(*other.mp4_moov_)) {
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

uint64 MediaInfo::file_size() const {
  return file_size_;
}
void MediaInfo::set_file_size(uint64 file_size) {
  file_size_ = file_size;
}

bool MediaInfo::seekable() const {
  return seekable_;
}
void MediaInfo::set_seekable(bool seekable) {
  seekable_ = seekable;
}

bool MediaInfo::pausable() const {
  return pausable_;
}
void MediaInfo::set_pausable(bool pausable) {
  pausable_ = pausable;
}

const vector<MediaInfo::Frame>& MediaInfo::frames() const {
  return frames_;
}
vector<MediaInfo::Frame>* MediaInfo::mutable_frames() {
  return &frames_;
}
void MediaInfo::set_frames(const vector<f4v::FrameHeader*>& frames) {
  frames_.clear();
  frames_.reserve(frames.size());
  for ( uint32 i = 0; i < frames.size(); i++ ) {
    frames_.push_back(Frame(
        frames[i]->type() == f4v::FrameHeader::AUDIO_FRAME,
        frames[i]->size(),
        frames[i]->decoding_timestamp(),
        frames[i]->composition_offset_ms(),
        frames[i]->is_keyframe()));
  }
}

const rtmp::CMixedMap& MediaInfo::flv_extra_metadata() const {
  return flv_extra_metadata_;
}
rtmp::CMixedMap* MediaInfo::mutable_flv_extra_metadata() {
  return &flv_extra_metadata_;
}

const f4v::MoovAtom* MediaInfo::mp4_moov() const {
  return mp4_moov_;
}
f4v::MoovAtom* MediaInfo::mutable_mp4_moov() {
  return mp4_moov_;
}
void MediaInfo::set_mp4_moov(const f4v::MoovAtom& moov) {
  delete mp4_moov_;
  mp4_moov_ = new f4v::MoovAtom(moov);
}


string MediaInfo::ToString() const {
  ostringstream oss;
  oss << "MediaInfo{"
      << " \n\t audio_: " << (has_audio() ? audio_.ToString(",\n\t\t ").c_str() : "NONE")
      << ",\n\t video_: " << (has_video() ? video_.ToString(",\n\t\t ").c_str() : "NONE")
      << ",\n\t duration_ms_: " << duration_ms_
      << ",\n\t file_size_: " << file_size_
      << ",\n\t seekable_: " << strutil::BoolToString(seekable_)
      << ",\n\t pausable_: " << strutil::BoolToString(pausable_)
      << ",\n\t frames_: #" << frames_.size()
      << ",\n\t flv_extra_metadata_: " << flv_extra_metadata_.ToString()
      << ",\n\t mp4_moov_: " << (mp4_moov_ == NULL ? "NULL" : "FOUND") << "}";
  return oss.str();
}

}
