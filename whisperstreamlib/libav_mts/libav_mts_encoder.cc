#include <whisperstreamlib/libav_mts/libav_mts_encoder.h>

#ifndef UINT64_C
#define UINT64_C(value) value ## ULL
#endif

#ifndef INT64_C
#define INT64_C(value) value ## LL
#endif

extern "C" {
#pragma GCC diagnostic ignored "-Wall"
#include __AV_FORMAT_INCLUDE_FILE
#include __AV_CODEC_INCLUDE_FILE
#pragma GCC diagnostic warning "-Wall"
}

namespace streaming {
namespace libav_mts {

void GlobalInit() {
  av_register_all();
}

Encoder::Encoder()
  : video_st_(NULL),
    audio_st_(NULL),
    video_filter_(NULL),
    io_buf_(NULL),
    headers_written_(false),
    crt_out_(NULL) {
}
Encoder::~Encoder() {
  Cleanup();
  CHECK_NULL(video_st_);
  CHECK_NULL(audio_st_);
  CHECK_NULL(video_filter_);
  CHECK_NULL(io_buf_);
  CHECK_NULL(crt_out_);
}

bool Encoder::IsInitialized() const {
  return av_context_ != NULL;
}

bool Encoder::Initialize(const MediaInfo& info, io::MemoryStream* out) {
  if ( IsInitialized() ) {
    LOG_WARNING << "Re Initialize(), closing old format";
    Finalize(out);
  }

  // the output format is Mpeg Transport Stream
  const char* shortname = "mpegts";
  AVOutputFormat *fmt = av_guess_format(shortname, NULL, NULL);
  if ( fmt == NULL ) {
    LOG_ERROR << "Could not deduce output format from shortname: " << shortname;
    return false;
  }

  // allocate the output media context
  av_context_ = avformat_alloc_context();
  if ( av_context_ == NULL ) {
    LOG_ERROR << "avformat_alloc_context() failed";
    return false;
  }
  av_context_->oformat = fmt;

  LOG_WARNING << "## Detected Output format"
                 ", video: " << CodecIDName(fmt->video_codec)
              << ", audio: " << CodecIDName(fmt->audio_codec);

  // Create internal Buffer for FFmpeg:
  static const uint32 kIOBufSize = 32 * 1024;
  CHECK_NULL(io_buf_);
  io_buf_ = (uint8*)av_malloc(kIOBufSize * sizeof(uint8));

  // Allocate the AVIOContext:
  // The fourth parameter (pStream) is a user parameter which will be passed to our callback functions
  av_context_->pb = avio_alloc_context(
      io_buf_, kIOBufSize,// internal Buffer and its size
      1,                  // bWriteable (1=true,0=false)
      this,               // user data; will be passed to our callback functions
      NULL,               // Read callback, not needed
      &Encoder::IOWrite,  // Write callback
      NULL);              // Seek Callback, not needed

  CHECK_NULL(video_st_);
  if ( info.has_video() ) {
    if ( !AddVideoStream(info.video()) ) {
      LOG_ERROR << "AddVideoStream() failed";
      return false;
    }
    // set output format's video codec
    fmt->video_codec = video_st_->codec->codec_id;
    // maybe initialize a video filter. Needed to transform H264 frames
    // from 'NALU with size (aka: mp4 H264)'
    // to 'NALU with start code (aka: annex B H264)'
    if ( info.video().format_ == streaming::MediaInfo::Video::FORMAT_H264 &&
         !info.video().h264_nalu_start_code_ ) {
      video_filter_ = av_bitstream_filter_init("h264_mp4toannexb");
      if ( video_filter_ == NULL ) {
        LOG_ERROR <<  "Cannot open the 'h264_mp4toannexb' filter";
        return false;
      }
    }
  }
  CHECK_NULL(audio_st_);
  if ( info.has_audio() ) {
    if ( !AddAudioStream(info.audio()) ) {
      LOG_ERROR << "AddAudioStream() failed";
      return false;
    }
    // set output format's audio codec
    fmt->audio_codec = audio_st_->codec->codec_id;
  }

  //av_dump_format(av_context_, 0, "", 1);

  // write the stream header to the output stream
  crt_out_ = out;
  int result = avformat_write_header(av_context_, NULL);
  crt_out_ = NULL;
  if ( result < 0 ) {
    LOG_ERROR << "avformat_write_header() failed: " << av_err2str(result);
  }
  headers_written_ = true;

  return true;
}
bool Encoder::Finalize(io::MemoryStream* out) {
  if ( !IsInitialized() ) {
    LOG_ERROR << "Finalize() failed, Encoder is not initialized";
    return false;
  }

  // write the trailer (before closing the Codec Context)
  if ( headers_written_ ) {
    int result = av_write_trailer(av_context_);
    if ( result < 0 ) {
      LOG_ERROR << "av_write_trailer() failed: " << av_err2str(result);
      return false;
    }
    headers_written_ = false;
  }
  // close the Codec Context and everything else
  Cleanup();
  return true;
}

bool Encoder::WriteTag(const Tag& tag, int64 ts, io::MemoryStream* out) {
  if ( tag.type() == Tag::TYPE_MEDIA_INFO ) {
    return Initialize(static_cast<const MediaInfoTag&>(tag).info(), out);
  }
  if ( tag.Data() == NULL ) {
    return true; // ignore non media tag (like: SourceStarted, CuePoint, ...)
  }
  if ( !IsInitialized() ) {
    LOG_ERROR << "WriteTag() failed, Encoder is not initialized!";
    return false;
  }
  // establish the output stream (audio or video)
  AVStream* stream = NULL;
  if      ( tag.is_audio_tag() ) { stream = audio_st_; }
  else if ( tag.is_video_tag() ) { stream = video_st_; }
  else { LOG_WARNING << "Ignoring tag: " << tag.ToString(); return true; }

  if ( stream == NULL ) {
    LOG_ERROR << "No " << (tag.is_audio_tag() ? "audio" : "video")
              << " in media info, cannot serialize tag: " << tag.ToString();
    return false;
  }
  if ( tag.Data() == NULL ) {
    LOG_WARNING << "Ignoring tag without data: " << tag.ToString();
    return true;
  }

  AVPacket pkt = {0}; // data and size must be 0;
  av_init_packet(&pkt);
  if ( tag.can_resync() ) {
    pkt.flags |= AV_PKT_FLAG_KEY;
  }
  pkt.pos = -1;
  pkt.stream_index = stream->index;
  pkt.size = tag.Data()->Size();
  pkt.data = new uint8[pkt.size];
  tag.Data()->Peek(pkt.data, pkt.size);
  pkt.destruct = &Encoder::DestructPacket;
  pkt.dts = ts * stream->time_base.den / stream->time_base.num / 1000;
  if ( tag.is_audio_tag() ) {
    pkt.pts = AV_NOPTS_VALUE;
  } else {
    pkt.pts = pkt.dts + tag.composition_offset_ms() * stream->time_base.den / stream->time_base.num / 1000;
  }
  pkt.duration = tag.duration_ms();

  // maybe apply a filter to this frame
  if ( tag.is_video_tag() && video_filter_ != NULL ) {
    uint8_t* new_data = NULL;
    int new_size = 0;
    int result = av_bitstream_filter_filter(video_filter_, stream->codec, NULL,
                                            &new_data, &new_size,
                                            pkt.data, pkt.size,
                                            pkt.flags & AV_PKT_FLAG_KEY);
    if ( result < 0 ) {
      LOG_ERROR << "av_bitstream_filter_filter() failed, " << av_err2str(result);
      return false;
    }
    delete pkt.data;
    pkt.data = new_data;
    pkt.size = new_size;
  }

  crt_out_ = out;
  //int ret = av_interleaved_write_frame(av_context_, &pkt);
  int ret = av_write_frame(av_context_, &pkt);
  crt_out_ = NULL;
  if ( ret != 0 ) {
    LOG_ERROR << "av_interleaved_write_frame() failed: " << av_err2str(ret);
    return false;
  }
  return true;
}

bool Encoder::AddAudioStream(const MediaInfo::Audio& inf) {
  CodecID cid = CODEC_ID_NONE;
  switch ( inf.format_ ) {
    case streaming::MediaInfo::Audio::FORMAT_AAC: cid = CODEC_ID_AAC; break;
    case streaming::MediaInfo::Audio::FORMAT_MP3: cid = CODEC_ID_MP3; break;
  }

  CHECK_NULL(audio_st_);
  audio_st_ = avformat_new_stream(av_context_, 0);
  if ( audio_st_ == NULL ) {
    LOG_ERROR << "avformat_new_stream() failed";
    return false;
  }
  AVCodecContext *pcc = audio_st_->codec;
  avcodec_get_context_defaults3(pcc, NULL);
  pcc->codec_type = AVMEDIA_TYPE_AUDIO;
  pcc->codec_id = cid;
  pcc->time_base.num = 1;
  pcc->time_base.den = 1000;
  pcc->sample_rate = inf.sample_rate_;
  pcc->frame_size = 1024;
  pcc->sample_fmt = AV_SAMPLE_FMT_S16;
  pcc->channels = inf.channels_;

  // set audio codec extradata
  if ( inf.format_ == streaming::MediaInfo::Audio::FORMAT_AAC ) {
    audio_st_->codec->extradata_size = 2;
    audio_st_->codec->extradata = new uint8[2 + FF_INPUT_BUFFER_PADDING_SIZE];
    ::memcpy(audio_st_->codec->extradata, inf.aac_config_, 2);
    audio_st_->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
  }

  return true;
}
bool Encoder::AddVideoStream(const MediaInfo::Video& inf) {
  CodecID cid = CODEC_ID_NONE;
  switch ( inf.format_ ) {
    case streaming::MediaInfo::Video::FORMAT_H263: cid = CODEC_ID_H263; break;
    case streaming::MediaInfo::Video::FORMAT_H264: cid = CODEC_ID_H264; break;
    case streaming::MediaInfo::Video::FORMAT_ON_VP6: cid = CODEC_ID_VP6; break;
  };

  CHECK_NULL(video_st_);
  video_st_ = avformat_new_stream(av_context_, 0);
  if ( video_st_ == NULL ) {
    LOG_ERROR << "avformat_new_stream() failed";
    return false;
  }
  AVCodecContext *pcc = video_st_->codec;
  avcodec_get_context_defaults3(pcc, NULL);
  pcc->codec_type = AVMEDIA_TYPE_VIDEO;
  pcc->codec_id = cid;
  pcc->width = inf.width_;
  pcc->height = inf.height_;
  pcc->time_base.num = 1;
  pcc->time_base.den = 1000;

  // set video codec extradata
  if ( inf.format_ == streaming::MediaInfo::Video::FORMAT_H264 &&
       !inf.h264_avcc_.empty() ) {
    video_st_->codec->extradata_size = inf.h264_avcc_.size();
    video_st_->codec->extradata = new uint8[video_st_->codec->extradata_size +
                                            FF_INPUT_BUFFER_PADDING_SIZE];
    ::memcpy(video_st_->codec->extradata,
             inf.h264_avcc_.c_str(),
             video_st_->codec->extradata_size);
    video_st_->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
  }

  return true;
}

void Encoder::Cleanup() {
  if ( av_context_ != NULL ) {
    for ( uint32 i = 0; i < av_context_->nb_streams; i++ ) {
      av_freep(&av_context_->streams[i]->codec);
      av_freep(&av_context_->streams[i]);
    }
    video_st_ = NULL;
    audio_st_ = NULL;

    av_free(av_context_->pb);
    av_context_->pb = NULL;

    av_free(av_context_);
    av_context_ = NULL;
  }

  if ( video_filter_ != NULL ) {
    av_bitstream_filter_close(video_filter_);
    video_filter_ = NULL;
  }

  av_free(io_buf_);
  io_buf_ = NULL;
}
string Encoder::av_err2str(int err) {
  char str_err[256] = {0,};
  av_strerror(err, str_err, sizeof(str_err));
  return str_err;
}
const char* Encoder::CodecIDName(uint32 codec_id) {
  AVCodec* a = avcodec_find_decoder((CodecID)codec_id);
  return a == NULL ? "Unknown" : a->name;
}
void Encoder::DestructPacket(AVPacket* pkt) {
  delete pkt->data;
  pkt->data = NULL;
}

////////////////////////////////////////////////////////////////////////

Serializer::Serializer()
    : TagSerializer(MFORMAT_MTS),
      encoder_() {}
Serializer::~Serializer() {}

void Serializer::Initialize(io::MemoryStream* out) {
}
void Serializer::Finalize(io::MemoryStream* out) {
  encoder_.Finalize(out);
}
bool Serializer::SerializeInternal(const streaming::Tag* tag,
                                   int64 timestamp_ms,
                                   io::MemoryStream* out) {
  return encoder_.WriteTag(*tag, timestamp_ms, out);
}

}
}
